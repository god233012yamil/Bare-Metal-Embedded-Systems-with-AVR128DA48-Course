/**
 * @file    main.c
 * @brief   PWM-Controlled Power Module — application entry point.
 *
 * This is the top-level application layer for the AVR128DA48 PWM-Controlled
 * Power Module.  It owns the system initialization sequence, the cooperative
 * super-loop, and the control-loop state machine.  It must not access
 * hardware registers directly; all peripheral interaction goes through the
 * driver API (adc.h, tca.h, ac.h, usart.h, evsys.h).
 *
 * ==========================================================================
 * System overview
 * ==========================================================================
 *
 *  ???????????????????????????????????????????????????????????????????????
 *  ?  Application Layer (this file)                                      ?
 *  ?  - System init                  - Control-loop state machine        ?
 *  ?  - Cooperative scheduler tasks  - Telemetry formatting              ?
 *  ???????????????????????????????????????????????????????????????????????
 *  ?  Driver Layer  (adc / tca / ac / usart / evsys / scheduler / fifo)  ?
 *  ???????????????????????????????????????????????????????????????????????
 *  ?  Hardware (AVR128DA48 registers)                                    ?
 *  ???????????????????????????????????????????????????????????????????????
 *
 * ==========================================================================
 * Control-loop state machine
 * ==========================================================================
 *
 *                          ??????????
 *              ?????????????  IDLE  ??????????????????
 *              ?  reset    ??????????  AC falls low  ?
 *              ?               ? init OK             ?
 *              ?               ?                     ?
 *              ?         ????????????                ?
 *              ?         ? SAMPLING ?                ?
 *              ?         ????????????                ?
 *              ?    ADC ready ?                      ?
 *              ?              ?                      ?
 *              ?        ????????????                 ?
 *              ?        ? ADJUSTING?                 ?
 *              ?        ????????????                 ?
 *              ?   duty set  ?                       ?
 *              ?             ?                       ?
 *              ?      ??????????????  AC fires high  ?
 *              ?      ?  RUNNING   ???????????????????  (loop back to IDLE
 *              ?      ??????????????                      then re-enter)
 *              ?  fault    ?
 *              ?????????????
 *
 * ==========================================================================
 * Scheduler tasks (all non-blocking)
 * ==========================================================================
 *
 *  Task              Period       Action
 *  ?????????????     ??????????   ??????????????????????????????????????????
 *  Task_ADC          ~50 ms       Start an ADC conversion if SM is SAMPLING.
 *  Task_Control      ~100 ms      Run the control-loop SM step.
 *  Task_Telemetry    ~500 ms      Emit CSV telemetry over USART0.
 *  Task_LED          ~500 ms      Toggle the Curiosity Nano LED (heartbeat).
 *
 * ==========================================================================
 * Hardware resources (summary)
 * ==========================================================================
 *
 *  Peripheral  Pin   Function
 *  ??????????  ????  ?????????????????????????????????????????????????
 *  ADC0        PD2   Analogue input (potentiometer / signal)
 *  TCA0 WO0    PD0   PWM output (duty ? ADC reading)
 *  AC0         PD2   Threshold comparator (shared pin with ADC)
 *  USART0      PA0   TXD ? CDC-USB (telemetry)
 *  USART0      PA1   RXD ? CDC-USB (command echo, future use)
 *  LED         PC6   Heartbeat (active-low, Curiosity Nano)
 *  RTC PIT     —     Scheduler tick (~1024 Hz)
 *  EVSYS CH0   —     AC0_OUT ? ADC0 START (autonomous)
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 *
 * Target MCU : AVR128DA48
 * Board      : AVR128DA48 Curiosity Nano
 * DFP        : AVR-Dx 2.4.286
 * IDE        : Atmel Studio 7 / Microchip Studio
 * Toolchain  : AVR-GCC (GCC C Executable Project)
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "adc.h"
#include "tca.h"
#include "ac.h"
#include "usart.h"
#include "evsys.h"
#include "scheduler.h"
#include "fifo.h"

/* =========================================================================
 * Control-loop state machine
 * ========================================================================= */

/**
 * @brief States of the power-module control-loop FSM.
 */
typedef enum
{
    CTRL_STATE_IDLE      = 0,   /**< Initial state; waiting for first sample. */
    CTRL_STATE_SAMPLING  = 1,   /**< ADC conversion has been requested.        */
    CTRL_STATE_ADJUSTING = 2,   /**< ADC result ready; computing new duty.     */
    CTRL_STATE_RUNNING   = 3,   /**< Steady state; monitoring for AC event.    */
} CtrlState_t;

/** Current state of the control-loop FSM. */
static CtrlState_t g_ctrlState = CTRL_STATE_IDLE;

/** Most recently applied PWM duty cycle (for telemetry). */
static volatile uint16_t g_currentDuty = 0U;

/** Most recently read ADC value (for telemetry). */
static volatile uint16_t g_currentADC  = 0U;

/** Flag: AC threshold event pending (set by control task, app-level). */
static volatile bool g_acEventPending  = false;

/* =========================================================================
 * Forward declarations (static task functions)
 * ========================================================================= */

static void Task_ADC(void);
static void Task_Control(void);
static void Task_Telemetry(void);
static void Task_LED(void);
static void App_SystemInit(void);

/* =========================================================================
 * System initialization
 * ========================================================================= */

/**
 * @brief Configures the CPU clock, then initializes all drivers in order.
 *
 * Clock setup (register-level, no HAL):
 *  The AVR128DA48 boots with OSCHF at 4 MHz.  We reconfigure it to 24 MHz
 *  using the Configuration Change Protection (CCP) sequence.
 *
 * Driver initialization order matters:
 *  1. USART  — so startup messages can be sent as early as possible.
 *  2. ADC    — configures PD2 and VREF.
 *  3. AC     — shares PD2 (no conflict because digital buffer is off).
 *              Also configures its own VREF.ACREF field.
 *  4. TCA    — configures PD0 and starts PWM (0 % duty initially).
 *  5. EVSYS  — wires AC0_OUT ? ADC0 START (requires AC and ADC to exist).
 *  6. SCHED  — starts the RTC PIT tick.
 *  7. LED    — configure after all peripherals to avoid pin conflicts.
 */
static void App_SystemInit(void)
{
    /* ---- CPU clock: switch OSCHF to 24 MHz ---- */
    /* CCP write sequence: write the signature, then the target register
     * within 4 CPU cycles.  The CLKCTRL_FREQSEL field selects the OSCHF
     * frequency (0x09 = 24 MHz, per AVR128DA48 data sheet §9). */
    CCP = CCP_IOREG_gc;
    CLKCTRL.OSCHFCTRLA = CLKCTRL_FRQSEL_24M_gc;

    /* Wait for the clock switch to complete. */
    while (CLKCTRL.MCLKSTATUS & CLKCTRL_SOSC_bm) { /* wait */ }

    /* ---- LED pin (PC6, active-low) ---- */
    LED_PORT.DIRSET  = LED_PIN_bm;
    LED_PORT.OUTSET  = LED_PIN_bm;     /* off initially (active-low) */

    /* ---- Driver initialization ---- */
    USART_Init();
    ADC_Init();
    AC_Init();
    TCA_Init();
    EVSYS_Init();
    SCHED_Init();
}

/* =========================================================================
 * Scheduler task implementations
 * ========================================================================= */

/**
 * @brief ADC task — fires every ~50 ms.
 *
 * In the SAMPLING state the control task has requested a new measurement.
 * This task fires the ADC conversion; the result is captured by the RESRDY
 * ISR inside adc.c and will be collected by the control task on its next
 * execution.
 *
 * Outside the SAMPLING state the task does nothing, avoiding unnecessary
 * conversions and power consumption.
 */
static void Task_ADC(void)
{
    if (g_ctrlState == CTRL_STATE_SAMPLING)
    {
        ADC_StartConversion();
    }
}

/**
 * @brief Control-loop task — fires every ~100 ms.
 *
 * Implements a one-step advance of the control FSM per invocation.
 *
 * State transitions:
 *  IDLE      ? SAMPLING   unconditionally on first run.
 *  SAMPLING  ? ADJUSTING  when ADC result is ready.
 *  ADJUSTING ? RUNNING    after computing and applying a new duty cycle.
 *  RUNNING   ? SAMPLING   on the next tick (continuous loop).
 *  RUNNING   ? IDLE       when AC fires high (threshold exceeded); duty
 *                          is clamped and output is briefly inhibited before
 *                          re-entering the measurement cycle.
 *
 * PWM duty scaling:
 *   duty = adc_raw >> CTRL_ADC_TO_PWM_SHIFT
 *   Maps 0–4095 (12-bit ADC) ? 0–1023 (10-bit effective PWM range).
 */
static void Task_Control(void)
{
    uint16_t adcVal;
    uint16_t newDuty;

    switch (g_ctrlState)
    {
        /* ------------------------------------------------------------------ */
        case CTRL_STATE_IDLE:
        /* ------------------------------------------------------------------ */
            /* First entry: enable PWM output and start the measurement cycle. */
            TCA_EnableOutput();
            ADC_ClearResultFlag();
            g_ctrlState = CTRL_STATE_SAMPLING;
            break;

        /* ------------------------------------------------------------------ */
        case CTRL_STATE_SAMPLING:
        /* ------------------------------------------------------------------ */
            /* Wait for the ADC ISR to deliver a result.
             * Task_ADC() is responsible for triggering the conversion;
             * we simply poll the ready flag here. */
            if (ADC_IsResultReady())
            {
                g_ctrlState = CTRL_STATE_ADJUSTING;
            }
            break;

        /* ------------------------------------------------------------------ */
        case CTRL_STATE_ADJUSTING:
        /* ------------------------------------------------------------------ */
            /* Collect and clear the ADC sample. */
            adcVal = ADC_GetResult();
            ADC_ClearResultFlag();
            g_currentADC = adcVal;

            /* Proportional scaling: 12-bit ADC ? 10-bit PWM. */
            newDuty = (uint16_t)(adcVal >> CTRL_ADC_TO_PWM_SHIFT);

            /* Apply the new duty cycle via the TCA driver. */
            TCA_SetDuty(newDuty);
            g_currentDuty = newDuty;

            g_ctrlState = CTRL_STATE_RUNNING;
            break;

        /* ------------------------------------------------------------------ */
        case CTRL_STATE_RUNNING:
        /* ------------------------------------------------------------------ */
            /* Check for an AC threshold-crossing event set by the ISR
             * (via the ac.c driver flag). */
            if (AC_IsFlagSet())
            {
                AC_ClearFlag();
                g_acEventPending = true;

                if (AC_GetOutput())
                {
                    /* Input is above threshold: clamp duty to maximum and
                     * signal a high-power condition. */
                    TCA_SetDuty(PWM_DUTY_MAX);
                    g_currentDuty = PWM_DUTY_MAX;

                    USART_SendString("[AC] Threshold HIGH — duty clamped\r\n");
                }
                else
                {
                    /* Input fell back below threshold: resume normal control. */
                    USART_SendString("[AC] Threshold LOW  — resuming control\r\n");
                }

                g_acEventPending = false;
            }

            /* Continuous measurement: loop back to sampling. */
            g_ctrlState = CTRL_STATE_SAMPLING;
            break;

        /* ------------------------------------------------------------------ */
        default:
        /* ------------------------------------------------------------------ */
            /* Defensive default: recover to IDLE. */
            g_ctrlState = CTRL_STATE_IDLE;
            break;
    }
}

/**
 * @brief Telemetry task — fires every ~500 ms.
 *
 * Emits a CSV-formatted line over USART0 for real-time monitoring via a
 * serial terminal (115200 8N1):
 *
 *   tick,adc,duty,ac_out,state\r\n
 *
 * Example line:
 *   512,2048,512,0,3\r\n
 */
static void Task_Telemetry(void)
{
    /* Snapshot volatile variables atomically to avoid torn reads. */
    uint16_t  tick;
    uint16_t  adc;
    uint16_t  duty;
    bool      acOut;
    uint8_t   state;

    cli();
    tick  = SCHED_GetTick();
    adc   = g_currentADC;
    duty  = g_currentDuty;
    sei();

    acOut = AC_GetOutput();
    state = (uint8_t)g_ctrlState;

    /* Emit CSV record. */
    USART_SendUInt16(tick);
    USART_SendByte(',');
    USART_SendUInt16(adc);
    USART_SendByte(',');
    USART_SendUInt16(duty);
    USART_SendByte(',');
    USART_SendByte(acOut ? '1' : '0');
    USART_SendByte(',');
    USART_SendByte((char)('0' + state));
    USART_SendString("\r\n");
}

/**
 * @brief LED heartbeat task — fires every ~500 ms.
 *
 * Toggles the active-low LED on PC6 of the Curiosity Nano.  A steady blink
 * at 1 Hz confirms the scheduler is running and the firmware has not locked
 * up.
 */
static void Task_LED(void)
{
    LED_PORT.OUTTGL = LED_PIN_bm;
}

/* =========================================================================
 * Application entry point
 * ========================================================================= */

/**
 * @brief Application entry point.
 *
 * Performs one-time hardware and driver initialization, registers all
 * scheduler tasks, then enters the infinite cooperative super-loop.
 *
 * @return Never returns (embedded super-loop).
 */
int main(void)
{
    /* ---- System initialization (clocks, peripherals, drivers) ---- */
    App_SystemInit();

    /* ---- Register cooperative tasks with the scheduler ---- */
    (void)SCHED_RegisterTask(Task_ADC,       TASK_ADC_PERIOD_TICKS);
    (void)SCHED_RegisterTask(Task_Control,   TASK_CTRL_PERIOD_TICKS);
    (void)SCHED_RegisterTask(Task_Telemetry, TASK_TELEM_PERIOD_TICKS);
    (void)SCHED_RegisterTask(Task_LED,       TASK_LED_PERIOD_TICKS);

    /* ---- Enable global interrupts ---- */
    sei();

    /* ---- Announce startup over USART ---- */
    USART_SendString("\r\nPWM Power Module v1.0 — AVR128DA48\r\n");
    USART_SendString("Format: tick,adc,duty,ac_out,state\r\n");

    /* ---- Cooperative super-loop ---- */
    while (1)
    {
        /* Dispatch any tasks that have become due since the last tick. */
        SCHED_Run();

        /* Process any inbound USART bytes (simple echo for now). */
        {
            uint8_t rxByte;
            while (USART_GetByte(&rxByte))
            {
                /* Echo received characters back to the terminal. */
                USART_SendByte(rxByte);
            }
        }
    }

    /* Unreachable — satisfies compiler. */
    return 0;
}