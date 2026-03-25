/*
 * main.c
 *
 * ADC Driver Lab - AVR128DA48 Curiosity Nano
 *
 * Demonstrates the ADC driver API with no register access in application code.
 *
 * Hardware setup (Curiosity Nano)
 * --------------------------------
 *   Analogue input : PD0 / AIN0
 *     Connect a voltage divider or potentiometer between VDD (3.3 V) and GND,
 *     with the wiper to PD0.  The on-board LED (PC6, active-low) blinks at a
 *     rate proportional to the ADC reading so you can verify operation without
 *     a scope.
 *
 *   UART output    : USART1 on PC0 (TX) / PC1 (RX) at 9600 baud
 *     The Curiosity Nano virtual COM port is connected to USART1 (PC0/PC1)
 *     via the on-board debugger.  Open a terminal at 9600-8-N-1 to see
 *     the live ADC readings.
 *
 * Clock : 4 MHz internal oscillator (power-on default for AVR128DA48).
 *
 * The application
 *   1. Configures the analogue pin (PD0) via the PORT driver before calling
 *      ADC_Init() - the only place GPIO registers appear is the pin setup.
 *   2. Calls ADC_Init() once.
 *   3. Loops: reads AIN0, converts to mV, sends over UART, blinks LED.
 *
 * All ADC interaction uses the driver API exclusively (ADC_Init, ADC_Read,
 * ADC_ReadChannel, ADC_ToMillivolts).
 */

#include <avr/io.h>
#include <util/delay.h>
#include "adc.h"

/* -----------------------------------------------------------------------
 * Board constants
 * ----------------------------------------------------------------------- */
#define F_CPU_HZ          4000000UL   /* 4 MHz internal oscillator        */
#define UART_BAUD         9600UL

/* Curiosity Nano LED: PC6, active-low */
#define LED_PORT          PORTC
#define LED_PIN_bm        PIN6_bm

/* USART1 on PC0 (TX), PC1 (RX) - Curiosity Nano virtual COM */
#define DEBUG_USART       USART1
#define DEBUG_TX_PORT     PORTC
#define DEBUG_TX_PIN_bm   PIN0_bm

/* ADC channel used in the demo */
#define DEMO_CHANNEL      ADC_CH_AIN0   /* PD0 */

/* Reference voltage matching ADC_REF_VDD (3300 mV on Curiosity Nano) */
#define VREF_MV           3300UL

/* -----------------------------------------------------------------------
 * Minimal UART driver (local - not a separate module to keep the lab
 * focused on the ADC driver pattern, but demonstrates the same principle:
 * application code calls UART_Init / UART_PrintStr / UART_PrintU16).
 * ----------------------------------------------------------------------- */

/* BAUD register value for async normal mode:
 *   BAUD_REG = (64 * F_CPU) / (16 * baud)  = (4 * F_CPU) / baud          */
#define USART_BAUD_VAL    ((uint16_t)((4UL * F_CPU_HZ) / UART_BAUD))

static void UART_Init(void)
{
    /* Configure TX pin as output */
    DEBUG_TX_PORT.DIRSET = DEBUG_TX_PIN_bm;

    DEBUG_USART.BAUD  = USART_BAUD_VAL;
    DEBUG_USART.CTRLC = USART_CMODE_ASYNCHRONOUS_gc
                      | USART_PMODE_DISABLED_gc
                      | USART_SBMODE_1BIT_gc
                      | USART_CHSIZE_8BIT_gc;
    DEBUG_USART.CTRLB = USART_TXEN_bm;
}

static void UART_SendByte(uint8_t byte)
{
    while (!(DEBUG_USART.STATUS & USART_DREIF_bm))
    {
        /* wait for data-register-empty */
    }
    DEBUG_USART.TXDATAL = byte;
}

static void UART_PrintStr(const char *str)
{
    while (*str)
    {
        UART_SendByte((uint8_t)*str++);
    }
}

/* Print a 16-bit unsigned decimal value followed by a unit string */
static void UART_PrintU16(uint16_t val, const char *unit)
{
    char buf[8];
    uint8_t i = 0;

    if (val == 0)
    {
        buf[i++] = '0';
    }
    else
    {
        uint16_t tmp = val;
        while (tmp)
        {
            buf[i++] = (char)('0' + (tmp % 10));
            tmp /= 10;
        }
        /* buf holds digits in reverse order - reverse in place */
        uint8_t lo = 0, hi = i - 1;
        while (lo < hi)
        {
            char t    = buf[lo];
            buf[lo++] = buf[hi];
            buf[hi--] = t;
        }
    }
    buf[i] = '\0';

    UART_PrintStr(buf);
    if (unit)
    {
        UART_PrintStr(unit);
    }
}

/* -----------------------------------------------------------------------
 * LED helpers
 * ----------------------------------------------------------------------- */
static void LED_Init(void)
{
    LED_PORT.DIRSET = LED_PIN_bm;
    LED_PORT.OUTSET = LED_PIN_bm;   /* active-low: start OFF */
}

static void LED_Toggle(void)
{
    LED_PORT.OUTTGL = LED_PIN_bm;
}

/* -----------------------------------------------------------------------
 * Analogue pin setup
 * PD0 is used as AIN0.  Its digital input buffer must be disabled to
 * reduce power and prevent spurious digital glitches on the ADC input.
 * This is the only place a PORT register is touched in the application;
 * ADC registers are handled exclusively inside the driver.
 * ----------------------------------------------------------------------- */
static void AnalogPin_Init(void)
{
    PORTD.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    /* ---------- Peripheral initialisation ---------- */

    LED_Init();
    UART_Init();
    AnalogPin_Init();

    /* Configure the ADC driver.
     * Reference : internal 2.048 V (suits a 0-3.3 V input well).
     * Channel   : AIN0 (PD0).
     * Prescaler : DIV4  ->  4 MHz / 4 = 1 MHz ADC clock.
     * Samples   : 1 (no accumulation - single conversion per call).       */
    const adc_config_t adcCfg =
    {
        .reference = ADC_REF_2V048,
        .channel   = DEMO_CHANNEL,
        .prescaler = ADC_PRESC_DIV4,
        .samples   = ADC_SAMPLES_1,
    };

    ADC_Init(&adcCfg);

    UART_PrintStr("\r\n=== ADC Driver Lab - AVR128DA48 ===\r\n");
    UART_PrintStr("Channel : AIN0 (PD0)\r\n");
    UART_PrintStr("Ref     : 2.048 V internal\r\n");
    UART_PrintStr("------------------------------------\r\n");

    /* ---------- Application loop ---------- */
    for (;;)
    {
        /* Read the ADC - API call only, no registers in main.c */
        uint16_t raw = ADC_ReadChannel(DEMO_CHANNEL);

        /* Convert raw count to millivolts using the driver utility */
        uint32_t mv = ADC_ToMillivolts(raw, 2048UL);   /* ref = 2048 mV */

        /* Report over UART */
        UART_PrintStr("ADC raw=");
        UART_PrintU16(raw,            " | ");
        UART_PrintStr("voltage=");
        UART_PrintU16((uint16_t)mv,   " mV\r\n");

        /* Blink LED: longer ON time for higher readings */
        LED_Toggle();
        uint8_t blink_ms = (uint8_t)(raw >> 4);   /* 0-255 ms */
        if (blink_ms < 50) blink_ms = 50;         /* minimum visible blink */
        for (uint8_t d = 0; d < blink_ms; d++)
        {
            _delay_ms(1);
        }
        LED_Toggle();
        _delay_ms(200);
    }

    return 0;   /* unreachable */
}