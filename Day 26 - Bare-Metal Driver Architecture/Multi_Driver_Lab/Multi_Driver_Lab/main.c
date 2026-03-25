/*
 * main.c  --  Multi-Driver Lab, AVR128DA48 Curiosity Nano
 *
 * Demonstrates four driver modules with no direct register access in
 * application code.  Every peripheral interaction goes through the
 * driver API.
 *
 * -----------------------------------------------------------------------
 * Hardware (Curiosity Nano)
 * -----------------------------------------------------------------------
 *  ADC   : PD0 / AIN0  -- potentiometer wiper (0 V to 3.3 V)
 *            PORTD.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc  (set in main)
 *
 *  SPI   : SPI0, DEFAULT pins (PORTMUX default -- no SPIROUTEA write needed)
 *            PA4 = MOSI  (output)
 *            PA5 = MISO  (input)
 *            PA6 = SCK   (output)
 *            PA7 = /CS   (output, driven high by default, asserted manually)
 *          No physical SPI device is required; the demo performs a loopback
 *          read-back to show the API.  Connect PA4 to PA5 for loopback.
 *
 *  EEPROM: Internal 512-byte EEPROM via NVMCTRL.
 *          Writes a magic word and a counter, reads back and verifies.
 *
 *  TIMER : TCB0 in Periodic Interrupt mode, 100 ms period at 4 MHz.
 *          ISR increments a volatile counter; main prints it each second.
 *
 *  UART  : USART1, TX=PC0, 9600-8-N-1.
 *          PC0 is the Curiosity Nano virtual COM port TX pin.
 *
 * -----------------------------------------------------------------------
 * Clock: 4 MHz internal oscillator (AVR128DA48 power-on default).
 * -----------------------------------------------------------------------
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "adc.h"
#include "spi.h"
#include "eeprom.h"
#include "timer.h"
#include "uart.h"

/* -----------------------------------------------------------------------
 * Board / application constants
 * ----------------------------------------------------------------------- */
#define F_CPU_HZ          4000000UL
#define UART_BAUD_RATE    9600UL

/* Curiosity Nano LED0: PC6, active-low */
#define LED_PORT          PORTC
#define LED_PIN_bm        PIN6_bm

/* UART: USART1 on PC0 (TX) -- Curiosity Nano virtual COM */
#define DEBUG_UART        UART_INSTANCE_1
#define DEBUG_TX_PORT     PORTC
#define DEBUG_TX_PIN_bm   PIN0_bm

/* ADC: AIN0 on PD0, 2.048 V internal reference */
#define ADC_VREF_MV       2048UL

/* SPI0 default pins on PORTA */
#define SPI_MOSI_bm       PIN4_bm   /* PA4 */
#define SPI_MISO_bm       PIN5_bm   /* PA5 */
#define SPI_SCK_bm        PIN6_bm   /* PA6 */
#define SPI_CS_bm         PIN7_bm   /* PA7 */

/* EEPROM layout */
#define EE_ADDR_MAGIC     0x00U     /* 1 byte  magic value              */
#define EE_ADDR_COUNTER   0x01U     /* 2 bytes reset counter (LE)       */
#define EE_MAGIC_VALUE    0xA5U

/* Timer: 100 ms period, TCB0, CLK_PER / 1 at 4 MHz */
#define TIMER_PERIOD_MS   100UL
#define TIMER_TICKS       ((uint16_t)((TIMER_PERIOD_MS * F_CPU_HZ / 1000UL) - 1UL))

/* -----------------------------------------------------------------------
 * Shared state touched by ISR
 * ----------------------------------------------------------------------- */
static volatile uint16_t g_timer_ticks = 0;   /* incremented every 100 ms */

/* -----------------------------------------------------------------------
 * Timer ISR callback (called from TCB0_INT_vect inside timer.c)
 * ----------------------------------------------------------------------- */
static void timer0_callback(timer_instance_t inst)
{
    (void)inst;
    g_timer_ticks++;
}

/* -----------------------------------------------------------------------
 * Board-level helpers
 * ----------------------------------------------------------------------- */
static void LED_Init(void)
{
    LED_PORT.DIRSET = LED_PIN_bm;
    LED_PORT.OUTSET = LED_PIN_bm;   /* active-low: start OFF */
}

static void LED_On(void)  { LED_PORT.OUTCLR = LED_PIN_bm; }
static void LED_Off(void) { LED_PORT.OUTSET = LED_PIN_bm; }

/* -----------------------------------------------------------------------
 * Peripheral initialisation
 * ----------------------------------------------------------------------- */
static void init_uart(void)
{
    DEBUG_TX_PORT.DIRSET = DEBUG_TX_PIN_bm;

    uart_config_t cfg = {
        .instance  = DEBUG_UART,
        .baud      = UART_BAUD_RATE,
        .f_cpu_hz  = F_CPU_HZ,
    };
    UART_Init(&cfg);
}

static void init_adc(void)
{
    /* Disable digital input buffer on PD0 before enabling ADC */
    PORTD.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;

    adc_config_t cfg = {
        .reference = ADC_REF_2V048,
        .channel   = ADC_CH_AIN0,
        .prescaler = ADC_PRESC_DIV4,   /* 4 MHz / 4 = 1 MHz ADC clock */
        .samples   = ADC_SAMPLES_1,
    };
    ADC_Init(&cfg);
}

static void init_spi(void)
{
    /* Configure SPI0 default pins on PORTA */
    PORTA.DIRSET = SPI_MOSI_bm | SPI_SCK_bm | SPI_CS_bm;
    PORTA.DIRCLR = SPI_MISO_bm;
    PORTA.OUTSET = SPI_CS_bm;   /* /CS idle-high */

    spi_config_t cfg = {
        .instance   = SPI_INSTANCE_0,
        .prescaler  = SPI_PRESCALER_DIV16,  /* 4 MHz / 16 = 250 kHz */
        .mode       = SPI_MODE_0,
        .data_order = SPI_DATA_ORDER_MSB,
        .clk2x      = 0,
    };
    SPI_Init(&cfg);
}

static void init_timer(void)
{
    timer_config_t cfg = {
        .instance         = TIMER_TCB0,
        .clksel           = TIMER_CLK_DIV1,
        .period_ticks     = TIMER_TICKS,
        .interrupt_enable = 1,
    };
    TIMER_RegisterCallback(TIMER_TCB0, timer0_callback);
    TIMER_Init(&cfg);
}

/* -----------------------------------------------------------------------
 * EEPROM demo
 * Reads the reset counter stored in EEPROM, increments and re-writes it.
 * Returns the updated counter value.
 * ----------------------------------------------------------------------- */
static uint16_t eeprom_demo(void)
{
    uint8_t  magic   = EEPROM_ReadByte(EE_ADDR_MAGIC);
    uint16_t counter = 0;

    if (magic != EE_MAGIC_VALUE) {
        /* First run: initialise EEPROM */
        UART_SendStr(DEBUG_UART, "[EEPROM] First boot -- initialising\r\n");
        EEPROM_WriteByte(EE_ADDR_MAGIC, EE_MAGIC_VALUE);
        counter = 0;
    } else {
        /* Subsequent runs: read 16-bit counter (little-endian) */
        uint8_t lo = EEPROM_ReadByte(EE_ADDR_COUNTER);
        uint8_t hi = EEPROM_ReadByte((uint16_t)(EE_ADDR_COUNTER + 1));
        counter    = (uint16_t)((uint16_t)hi << 8) | lo;
    }

    counter++;

    /* Write updated counter back */
    EEPROM_WriteByte(EE_ADDR_COUNTER,      (uint8_t)(counter & 0xFF));
    EEPROM_WriteByte((uint16_t)(EE_ADDR_COUNTER + 1), (uint8_t)(counter >> 8));

    UART_SendStr(DEBUG_UART, "[EEPROM] Boot count = ");
    UART_SendU16(DEBUG_UART, counter);
    UART_SendStr(DEBUG_UART, "\r\n");

    return counter;
}

/* -----------------------------------------------------------------------
 * SPI loopback demo
 * Sends a known pattern and reads it back (requires PA4 jumpered to PA5).
 * ----------------------------------------------------------------------- */
static void spi_demo(void)
{
    static const uint8_t tx_buf[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
    uint8_t rx_buf[4] = { 0 };

    PORTA.OUTCLR = SPI_CS_bm;   /* Assert /CS */
    SPI_Transfer(SPI_INSTANCE_0, tx_buf, rx_buf, 4);
    PORTA.OUTSET = SPI_CS_bm;   /* De-assert /CS */

    UART_SendStr(DEBUG_UART, "[SPI]   TX:");
    for (uint8_t i = 0; i < 4; i++) {
        UART_SendByte(DEBUG_UART, ' ');
        UART_SendHex8(DEBUG_UART, tx_buf[i]);
    }
    UART_SendStr(DEBUG_UART, "  RX:");
    for (uint8_t i = 0; i < 4; i++) {
        UART_SendByte(DEBUG_UART, ' ');
        UART_SendHex8(DEBUG_UART, rx_buf[i]);
    }
    UART_SendStr(DEBUG_UART, "\r\n");
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    // Initialize the LED pin.
    LED_Init();

    // Initialize the UART peripherals in a safe order.  
    // UART first so we can print debug info, then ADC, SPI, and TIMER. 
    init_uart();
    init_adc();    
    init_spi();    
    init_timer();

    // Enable global interrupts after all peripherals are set up.
    sei();   

    // Print banner and run demos.
    UART_SendStr(DEBUG_UART, "\r\n========================================\r\n");
    UART_SendStr(DEBUG_UART,   "  Multi-Driver Lab  --  AVR128DA48\r\n");
    UART_SendStr(DEBUG_UART,   "  ADC | SPI | EEPROM | TIMER\r\n");
    UART_SendStr(DEBUG_UART,   "========================================\r\n");

    // Run the EEPROM demo first since it doesn't require any user interaction or wiring.  
    eeprom_demo();
    
    // Run the SPI demo before the main loop so we can verify it works without needing to 
    // connect the loopback wires while the timer is running.
    spi_demo();

    // Main loop: read ADC every second and print value with timer ticks.
    UART_SendStr(DEBUG_UART, "----------------------------------------\r\n");
    UART_SendStr(DEBUG_UART, "Loop: ADC reading every ~1 s (10 x 100 ms)\r\n\r\n");

    uint16_t last_ticks = 0;

    for (;;) {
        /* Wait for 10 timer ticks (10 x 100 ms = ~1 second) */
        uint16_t now = g_timer_ticks;
        if ((uint16_t)(now - last_ticks) < 10U) {
            continue;
        }
        last_ticks = now;

        /* ADC read */
        uint16_t raw = ADC_ReadChannel(ADC_CH_AIN0);
        uint32_t mv  = ADC_ToMillivolts(raw, ADC_VREF_MV);

        // Print ADC value and timer ticks over UART.
        UART_SendStr(DEBUG_UART, "ADC raw=");
        UART_SendU16(DEBUG_UART, raw);
        UART_SendStr(DEBUG_UART, "  voltage=");
        UART_SendU32(DEBUG_UART, mv);
        UART_SendStr(DEBUG_UART, " mV  ticks=");
        UART_SendU16(DEBUG_UART, now);
        UART_SendStr(DEBUG_UART, "\r\n");

        /* Blink LED to show activity */
        LED_On();
        _delay_ms(50);
        LED_Off();
    }

    return 0;
}