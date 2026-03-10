/**
 * @file main.c
 * @brief Layer 4 - Application Entry Point for AVR128DA48 TWI Demo
 *
 * Demonstrates the four-layer I2C driver architecture:
 *   - Scans the I2C bus and prints discovered addresses via USART.
 *   - Writes and reads back a short string to/from a 24C02 EEPROM.
 *   - Reads the current time from a DS1307/DS3231 RTC.
 *   - Reads the temperature register from an LM75/TMP75 sensor.
 *
 * Hardware connections (AVR128DA48 Curiosity Nano):
 *   PA2 (SDA) ??? SDA of all I2C devices (with 4.7 k? pull-up to VCC)
 *   PA3 (SCL) ??? SCL of all I2C devices (with 4.7 k? pull-up to VCC)
 *   PC0 (TXD) ??? USB CDC virtual COM (via on-board debugger)
 *
 * USART0 is configured for 9600 baud at 4 MHz to print results.
 * Open a serial terminal at 9600-8-N-1 to observe output.
 *
 * Target:    AVR128DA48
 * Toolchain: Atmel/Microchip Studio 7, avr-gcc
 * Pack:      AVR-Dx Device Pack 2.4.286
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>

#include "twi_master.h"    /* Layer 2 – init and result types only   */
#include "i2c_devices.h"   /* Layer 3 – all device-level API calls   */

/* -----------------------------------------------------------------------
 * USART helpers – minimal printf redirect via USART0 (PC0 = TXD)
 * ----------------------------------------------------------------------- */

/** USART0 baud register value for 9600 baud at F_CPU = 4 MHz.
 *  BAUD = 64 * F_CPU / (16 * BAUD_RATE) = 64*4000000/(16*9600) ? 1667 */
#define USART_BAUD_9600  1667u

/**
 * @brief Initialise USART0 for 9600-8-N-1 transmit-only operation.
 *        PC0 is the TXD pin on the AVR128DA48 Curiosity Nano.
 */
static void usart_init(void)
{
    PORTC.DIRSET   = PIN0_bm;                 /* PC0 as output (TXD) */
    USART0.BAUD    = USART_BAUD_9600;         /* Set baud rate       */
    USART0.CTRLB   = USART_TXEN_bm;          /* Enable transmitter  */
}

/**
 * @brief Transmit a single character via USART0 (used by printf redirect).
 * @param c Character to send.
 * @param stream Unused FILE pointer (required by avr-libc signature).
 * @return 0 always.
 */
static int usart_putchar(char c, FILE *stream)
{
    (void)stream;                                  /* Suppress unused warning */
    while (!(USART0.STATUS & USART_DREIF_bm))
    {
        /* Wait for Data Register Empty */
    }
    USART0.TXDATAL = (uint8_t)c;                   /* Write byte to transmit  */
    return 0;
}

/** Redirect stdout to usart_putchar so printf() works out of the box. */
static FILE usart_stdout = FDEV_SETUP_STREAM(usart_putchar, NULL, _FDEV_SETUP_WRITE);

/* -----------------------------------------------------------------------
 * Helper: decode TWI result to human-readable string
 * ----------------------------------------------------------------------- */

/**
 * @brief Return a short descriptive string for a TWI_Result_t value.
 * @param r Result code.
 * @return Pointer to a string literal.
 */
static const char *result_str(TWI_Result_t r)
{
    switch (r)
    {
        case TWI_RESULT_OK:         return "OK";
        case TWI_RESULT_NACK_ADDR:  return "NACK_ADDR";
        case TWI_RESULT_NACK_DATA:  return "NACK_DATA";
        case TWI_RESULT_ARB_LOST:   return "ARB_LOST";
        case TWI_RESULT_BUS_ERROR:  return "BUS_ERROR";
        case TWI_RESULT_TIMEOUT:    return "TIMEOUT";
        case TWI_RESULT_BUSY:       return "BUSY";
        default:                    return "UNKNOWN";
    }
}

/* -----------------------------------------------------------------------
 * Demo: I2C bus scan
 * Probes all 128 valid 7-bit addresses and prints the ones that respond.
 * ----------------------------------------------------------------------- */

/**
 * @brief Scan the I2C bus and print addresses of responding devices.
 */
static void demo_bus_scan(void)
{
    printf("\r\n--- I2C Bus Scan ---\r\n");

    uint8_t found = 0u;

    /* Valid 7-bit addresses are 0x08..0x77 (reserved excluded) */
    for (uint8_t addr = 0x08u; addr <= 0x77u; addr++)
    {
        TWI_Result_t r = i2c_probe(addr);   /* Layer 3 API */
        if (r == TWI_RESULT_OK)
        {
            printf("  Found device at 0x%02X\r\n", addr);
            found++;
        }
        _delay_ms(1);   /* Short delay between probes */
    }

    if (found == 0u)
        printf("  No devices found.\r\n");
    else
        printf("  Total: %u device(s).\r\n", found);
}

/* -----------------------------------------------------------------------
 * Demo: 24Cxx EEPROM write / read-back
 * ----------------------------------------------------------------------- */

/**
 * @brief Write a short string to EEPROM page 0 and read it back.
 */
static void demo_eeprom(void)
{
    printf("\r\n--- EEPROM Demo ---\r\n");

    const char   msg[]  = "AVR-I2C";           /* 7 chars + NUL = 8 bytes */
    const uint8_t len   = (uint8_t)strlen(msg) + 1u;  /* Include NUL terminator */
    uint8_t       rd[8] = {0};

    /* Write to EEPROM word address 0x0000 */
    TWI_Result_t r = eeprom_write_page(0x0000u, (const uint8_t *)msg, len);
    printf("  Write '%s': %s\r\n", msg, result_str(r));

    if (r == TWI_RESULT_OK)
    {
        /* Read back from the same address */
        r = eeprom_read_bytes(0x0000u, rd, len);
        printf("  Read  '%s': %s\r\n", (char *)rd, result_str(r));
    }
}

/* -----------------------------------------------------------------------
 * Demo: DS1307/DS3231 RTC read
 * ----------------------------------------------------------------------- */

/**
 * @brief Read and print the current time from the RTC.
 */
static void demo_rtc(void)
{
    printf("\r\n--- RTC Demo ---\r\n");

    RTC_Time_t t;
    TWI_Result_t r = rtc_read_time(&t);   /* Layer 3 API */

    if (r == TWI_RESULT_OK)
    {
        /* Values are in BCD; convert for display with simple nibble decode */
        printf("  Time: %02X:%02X:%02X  Date: 20%02X-%02X-%02X\r\n",
               t.hours, t.minutes, t.seconds,
               t.year,  t.month,   t.date);
    }
    else
    {
        printf("  RTC read failed: %s\r\n", result_str(r));
    }
}

/* -----------------------------------------------------------------------
 * Demo: LM75 / TMP75 temperature sensor (register 0x00 = 2-byte temp)
 * ----------------------------------------------------------------------- */
#define I2C_ADDR_LM75   0x48u   /**< LM75 address with A2:A0 = 0b000 */

/**
 * @brief Read and print the temperature from an LM75/TMP75 sensor.
 */
static void demo_temperature(void)
{
    printf("\r\n--- LM75 Temperature Demo ---\r\n");

    uint8_t      raw[2] = {0};
    TWI_Result_t r      = sensor_read_reg(I2C_ADDR_LM75, 0x00u, raw, 2u);

    if (r == TWI_RESULT_OK)
    {
        /* Bits 15..7 of the 16-bit register are the 9-bit temperature.
         * MSB is the sign bit; resolution is 0.5 °C per LSB (bit 7). */
        int16_t raw16  = (int16_t)((raw[0] << 8) | raw[1]);
        int16_t temp_x2 = raw16 >> 7;              /* Signed 9-bit value × 2  */
        int8_t  whole   = (int8_t)(temp_x2 / 2);  /* Integer degrees         */
        uint8_t frac    = (uint8_t)((temp_x2 & 1) * 5u);  /* 0 or 5 (tenths) */
        printf("  Temperature: %d.%u C\r\n", whole, frac);
    }
    else
    {
        printf("  LM75 read failed: %s\r\n", result_str(r));
    }
}

/* -----------------------------------------------------------------------
 * main()
 * ----------------------------------------------------------------------- */

/**
 * @brief Application entry point.
 *        Initialises peripherals, enables interrupts, and runs demos
 *        in an infinite loop with a 5-second interval.
 * @return This function never returns (embedded main).
 */
int main(void)
{
    /* ----- System initialisation ----- */
    usart_init();                 /* Set up USART0 for printf output  */
    stdout = &usart_stdout;       /* Redirect printf to USART          */

    twi_master_init();            /* Initialise TWI master (Layer 2)  */

    sei();                        /* Enable global interrupts – required
                                   * for the TWI ISR to run             */

    printf("\r\n========================================\r\n");
    printf(" AVR128DA48 TWI (I2C) Driver Demo\r\n");
    printf(" Layers: HW -> Engine -> Device -> App\r\n");
    printf("========================================\r\n");

    /* ----- Main application loop ----- */
    for (;;)
    {
        demo_bus_scan();       /* Scan and list all I2C devices   */
        demo_eeprom();         /* Write/read-back EEPROM demo      */
        demo_rtc();            /* Read RTC time demo               */
        demo_temperature();    /* Read temperature sensor demo     */

        printf("\r\n[Waiting 5 s before next cycle...]\r\n");
        _delay_ms(5000);       /* 5 s gap between demonstration runs */
    }

    return 0;   /* Unreachable; satisfies compiler */
}
