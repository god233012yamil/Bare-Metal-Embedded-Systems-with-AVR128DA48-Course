/*
 * LED_StateMachine
 *
 * Target:  AVR128DA48 Curiosity Nano
 * Device:  AVR128DA48
 * Clock:   4 MHz (internal OSCHF, default)
 *
 * Hardware connections (Curiosity Nano):
 *   LED0   -> PC6  (active LOW, onboard LED)
 *   SW0    -> PC7  (active LOW, onboard button, internal pull-up)
 *   ADC in -> PD3  (AIN3 on ADC0)  -- connect a pot / voltage divider
 *
 * State machine:
 *   IDLE    -> LED OFF
 *   RUNNING -> LED blink      (~500 ms period, TCA0 OVF ISR)
 *   ERROR   -> LED fast blink (~125 ms period, TCA0 OVF ISR)
 *
 * Transitions:
 *   IDLE    + button press           -> RUNNING
 *   RUNNING + ADC result >= THRESHOLD -> ERROR
 *   ERROR   + button press           -> IDLE  (reset / recover)
 */

#define F_CPU 4000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/* Pin / threshold definitions                                         */
/* ------------------------------------------------------------------ */
#define LED_PIN         PIN6_bm          /* PC6 - active LOW           */
#define BTN_PIN         PIN7_bm          /* PC7 - active LOW, pull-up  */
#define ADC_CHANNEL     ADC_MUXPOS_AIN3_gc  /* PD3                     */

/*
 * ADC is 12-bit (0-4095).  VREF = VDD (~3.3 V).
 * Threshold = ~2.5 V  =>  (2.5/3.3)*4096 ~ 3103
 */
#define ADC_THRESHOLD   3103u

/*
 * TCA0 period register values for desired blink rates.
 * CLK = 4 MHz, prescaler DIV1024 => tick = 256 us
 * RUNNING  500 ms / 2  = 250 ms half-period  => 250000 / 256 ~ 977  => PER = 976
 * ERROR    125 ms / 2  = 62.5 ms half-period => 62500  / 256 ~ 244  => PER = 243
 */
#define TCA_PER_RUNNING  976u
#define TCA_PER_ERROR    243u

/* ------------------------------------------------------------------ */
/* State machine                                                       */
/* ------------------------------------------------------------------ */
typedef enum
{
    STATE_IDLE = 0,
    STATE_RUNNING,
    STATE_ERROR
} app_state_t;

static volatile app_state_t g_state      = STATE_IDLE;
static volatile bool        g_btn_event  = false;   /* set by pin-change ISR  */
static volatile bool        g_adc_ready  = false;   /* set by ADC ISR         */
static volatile uint16_t    g_adc_result = 0;

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/** 
 * @brief Turn the LED on.
 */
static inline void led_on(void)
{
    PORTC.OUTCLR = LED_PIN;     /* drive low -> LED on  */
}

/** 
 * @brief Turn the LED off.
 */
static inline void led_off(void)
{
    PORTC.OUTSET = LED_PIN;     /* drive high -> LED off */
}

/** 
 * @brief Toggle the LED state.
 */
static inline void led_toggle(void)
{
    PORTC.OUTTGL = LED_PIN;
}

/* ------------------------------------------------------------------ */
/* Peripheral initialisation                                           */
/* ------------------------------------------------------------------ */

/** 
 * @brief Initialise the system clock.
 */
static void clock_init(void)
{
    /* Default after reset: OSCHF at 4 MHz - nothing to change */
}

/** 
 * @brief Initialise GPIO pins.
 */
static void gpio_init(void)
{
    /* LED pin: output, start HIGH (LED off) */
    PORTC.OUTSET  = LED_PIN;
    PORTC.DIRSET  = LED_PIN;

    /* Button pin: input, pull-up enabled, sense falling edge (ISR) */
    PORTC.DIRCLR  = BTN_PIN;
    PORTC.PIN7CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;

    /* ADC input pin PD3: disable digital input buffer */
    PORTD.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
}

/** 
 * @brief Initialise TCA0 for normal mode with DIV1024 prescaler.
 */
static void tca0_init(void)
{
    /* Normal mode, DIV1024 prescaler, OVF interrupt enabled          */
    /* Timer starts disabled; state machine enables/reloads it        */
    TCA0.SINGLE.CTRLB   = TCA_SINGLE_WGMODE_NORMAL_gc;
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
    TCA0.SINGLE.PER     = TCA_PER_RUNNING;
    TCA0.SINGLE.CNT     = 0;
    TCA0.SINGLE.CTRLA   = TCA_SINGLE_CLKSEL_DIV1024_gc; /* not yet enabled */
}

/** 
 * @brief Start the TCA0 timer with the specified period.
 * @param period The timer period.
 */
static void tca0_start(uint16_t period)
{
    TCA0.SINGLE.CTRLA   &= ~TCA_SINGLE_ENABLE_bm;       /* stop            */
    TCA0.SINGLE.CNT      = 0;
    TCA0.SINGLE.PER      = period;
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;           /* clear flag      */
    TCA0.SINGLE.CTRLA   |=  TCA_SINGLE_ENABLE_bm;       /* start           */
}

/** 
 * @brief Stop the TCA0 timer.
 */
static void tca0_stop(void)
{
    TCA0.SINGLE.CTRLA   &= ~TCA_SINGLE_ENABLE_bm;
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}

/** 
 * @brief Initialise ADC0 for single-ended input on configured channel, with VREF = VDD.
 * ADC is set to 12-bit resolution, prescaler DIV16 (250 kHz ADC clock), and result-ready interrupt enabled.
 * ADC conversions are triggered manually in the main loop when in RUNNING state.
*/
static void adc_init(void)
{
    /* VREF = VDD */
    VREF.ADC0REF = VREF_REFSEL_VDD_gc;

    /* ADC0: 12-bit, prescaler DIV16 (4MHz/16=250kHz), enabled        */
    ADC0.CTRLC   = ADC_PRESC_DIV16_gc;
    ADC0.MUXPOS  = ADC_CHANNEL;
    ADC0.INTCTRL = ADC_RESRDY_bm;                        /* result-ready ISR */
    ADC0.CTRLA   = ADC_RESSEL_12BIT_gc | ADC_ENABLE_bm;
}

/**
 * @brief Start an ADC conversion on the configured channel.  
 * Result will be handled in ISR.
 */
static void adc_start_conversion(void)
{
    ADC0.COMMAND = ADC_STCONV_bm;
}

/* ------------------------------------------------------------------ */
/* State-machine transition helpers                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Enter idle state: stop timer, turn LED off, and set state variable.
 */
static void enter_idle(void)
{
    tca0_stop();
    led_off();
    g_state = STATE_IDLE;
    /* Kick off an ADC sample so we re-evaluate on next entry */
}

/**
 * @brief Enter running state: start normal blinking and ADC monitoring.
 */
static void enter_running(void)
{
    g_state = STATE_RUNNING;
    led_off();                  /* start with LED off, timer will toggle */
    tca0_start(TCA_PER_RUNNING);
    adc_start_conversion();     /* begin monitoring ADC */
}

/**
 * @brief Enter error state: stop normal blinking, start fast blinking, and turn LED on.
 */
static void enter_error(void)
{
    g_state = STATE_ERROR;
    led_off();
    tca0_start(TCA_PER_ERROR);
}

/* ------------------------------------------------------------------ */
/* ISR: TCA0 overflow - toggle LED                                     */
/* ------------------------------------------------------------------ */
ISR(TCA0_OVF_vect)
{
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;           /* clear flag */
    led_toggle();
}

/* ------------------------------------------------------------------ */
/* ISR: PORTC falling edge - button pressed                            */
/* ------------------------------------------------------------------ */
ISR(PORTC_PORT_vect)
{
    /* Clear the interrupt flag for PC7 */
    PORTC.INTFLAGS = BTN_PIN;
    g_btn_event    = true;
}

/* ------------------------------------------------------------------ */
/* ISR: ADC0 result ready                                              */
/* ------------------------------------------------------------------ */
ISR(ADC0_RESRDY_vect)
{
    ADC0.INTFLAGS  = ADC_RESRDY_bm;                     /* clear flag  */
    g_adc_result   = ADC0.RES;
    g_adc_ready    = true;
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    // Clock setup is not strictly needed for this demo since we use the default 4 MHz OSCHF,
    // but it's good practice to have a dedicated init function for it.
    clock_init();
    
    // GPIO setup is needed in all states for button input and LED output
    gpio_init();
    
    // Timer setup is done in init, but timer is not started until
    // we enter RUNNING or ERROR states
    tca0_init();
    
    // ADC setup is needed in all states to monitor the input 
    // and trigger error state when threshold is exceeded
    adc_init();

    // Enable global interrupts after all peripheral setup is done
    sei();

    /* Start in IDLE state */
    enter_idle();

    while (1)
    {
        /* -------- Button event ----------------------------------- */
        if (g_btn_event)
        {
            g_btn_event = false;

            switch (g_state)
            {
                case STATE_IDLE:
                    enter_running();
                    break;

                case STATE_RUNNING:
                    /* Button has no defined role while running     */
                    break;

                case STATE_ERROR:
                    enter_idle();
                    break;

                default:
                    enter_idle();
                    break;
            }
        }

        /* -------- ADC result ------------------------------------- */
        if (g_adc_ready)
        {
            g_adc_ready = false;

            if (g_state == STATE_RUNNING)
            {
                if (g_adc_result >= ADC_THRESHOLD)
                {
                    enter_error();
                }
                else
                {
                    /* Keep sampling while running */
                    adc_start_conversion();
                }
            }
        }
    }
}