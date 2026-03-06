/**
 * @file    spi_driver.c
 * @brief   SPI0 driver implementation for the AVR128DA48 (AVR-Dx family).
 *
 * See spi_driver.h for the full API description and usage examples.
 *
 * Implementation notes
 * ---------------------
 * - Blocking transfers busy-wait on SPI0.INTFLAGS.IF with a simple cycle
 *   counter used as a timeout guard.  No hardware timer is consumed.
 *
 * - Non-blocking transfers use two fixed-size ring buffers (TX and RX) and
 *   the SPI0 Transfer-Complete interrupt (vector SPI0_INT_vect).  The ISR
 *   pops one byte from the TX ring, writes it to SPI0.DATA, and pushes the
 *   previously received byte into the RX ring.  This matches the AVR-Dx
 *   SPI buffer-mode operation described in the data sheet §24.
 *
 * - All ring-buffer index arithmetic uses bitwise AND with a power-of-2 mask
 *   (SPI_BUF_MASK) to avoid division on an 8-bit MCU.
 *
 * Register map (AVR128DA48, io.h from AVR-Dx 2.4.286)
 * -----------------------------------------------------
 *   SPI0.CTRLA   – CLK2X, MASTER, DORD, ENABLE, PRESC
 *   SPI0.CTRLB   – BUFEN, BUFWR, SSD, MODE
 *   SPI0.INTCTRL – RXCIE, TXCIE, DREIE, SSIE, IE
 *   SPI0.INTFLAGS – RXCIF, TXCIF, DREIF, SSIF, IF (legacy / non-buffered)
 *   SPI0.DATA    – 8-bit data register (read = RX, write = TX)
 *
 * @author  Yamil Garcia
 * @date    2026-02-17
 */

#include "spi_driver.h"

/* =========================================================================
 * Private macros
 * ========================================================================= */

/** Mask used for power-of-2 ring-buffer wrap-around arithmetic. */
#define SPI_BUF_MASK    ((SPI_BUFFER_SIZE) - 1U)

/* Compile-time check: SPI_BUFFER_SIZE must be a non-zero power of two.      */
#if (SPI_BUFFER_SIZE == 0U) || ((SPI_BUFFER_SIZE & SPI_BUF_MASK) != 0U)
#  error "SPI_BUFFER_SIZE must be a power of 2 (e.g. 16, 32, 64)"
#endif

/* =========================================================================
 * Private types – ring buffer
 * ========================================================================= */

/**
 * @brief Single-producer / single-consumer ring buffer.
 *
 * head – index of the NEXT byte to write (producer advances head).
 * tail – index of the NEXT byte to read  (consumer advances tail).
 * When head == tail the buffer is empty.
 * The buffer is full when ((head + 1) & mask) == tail.
 */
typedef struct
{
    volatile uint8_t  buf[SPI_BUFFER_SIZE];  /**< Storage array              */
    volatile uint8_t  head;                  /**< Write index (0-based)      */
    volatile uint8_t  tail;                  /**< Read  index (0-based)      */
} RingBuffer_t;

/* =========================================================================
 * Private state – file-scope (static) variables
 * ========================================================================= */

/** TX ring buffer – loaded by SPI_StartTransfer(), drained by the ISR.  */
static RingBuffer_t s_txBuf;

/** RX ring buffer – filled by the ISR, drained by SPI_ReadNonBlocking(). */
static RingBuffer_t s_rxBuf;

/** Number of bytes remaining in the current non-blocking transfer.       */
static volatile size_t s_txRemaining;

/** True while a non-blocking transfer is in progress.                    */
static volatile bool s_transferActive;

/** Cumulative count of timeout errors (blocking mode only).              */
static volatile uint32_t s_timeoutCount;

/* =========================================================================
 * Private helper prototypes
 * ========================================================================= */

static inline void    RingBuf_Init(RingBuffer_t *rb);
static inline bool    RingBuf_IsEmpty(const RingBuffer_t *rb);
static inline bool    RingBuf_IsFull(const RingBuffer_t *rb);
static inline uint8_t RingBuf_Pop(RingBuffer_t *rb);
static inline void    RingBuf_Push(RingBuffer_t *rb, uint8_t byte);

/* =========================================================================
 * Public API – initialisation
 * ========================================================================= */

void SPI_Init(SPI_Mode_t mode, SPI_Prescaler_t prescaler, bool clk2x)
{
    /* ---- 1. Configure PORTC pin directions --------------------------------
     * On the AVR128DA48 SPI0 is mapped to PORTC by default (PORTMUX reset
     * value = 0x00 → SPI0 on PORTA):
     *   PA6 = SCK  (output in master mode)
     *   PA5 = MISO (input  in master mode)
     *   PA4 = MOSI (output in master mode)
     *   PA7 = SS   (not used here; handled externally as GPIO CS)
     * -------------------------------------------------------------------- */
    PORTA.DIRSET = PIN6_bm | PIN4_bm;   /* SCK and MOSI → outputs          */
    PORTA.DIRCLR = PIN5_bm;             /* MISO         → input (high-z)   */

    /* ---- 2. Configure SPI0.CTRLB (Mode and Slave-Select Disable) ---------
     * SSD = 1 → hardware SS control disabled; we manage CS as plain GPIO.
     * MODE[1:0] sets clock polarity (CPOL) and phase (CPHA).
     * -------------------------------------------------------------------- */
    SPI0.CTRLB = SPI_SSD_bm                     /* disable hardware SS pin  */
               | (((uint8_t)mode) & SPI_MODE_gm);/* CPOL/CPHA mode          */

    /* ---- 3. Configure SPI0.CTRLA (Master enable, prescaler, CLK2X) -------
     * Writing ENABLE last is safe; the peripheral starts only after ENABLE
     * is set.  Do NOT set DORD here (LSB-first) unless your device requires
     * it; most sensors use MSB-first (DORD = 0, default).
     * -------------------------------------------------------------------- */
    SPI0.CTRLA = SPI_MASTER_bm                            /* master mode    */
               | ((((uint8_t)prescaler) << SPI_PRESC_gp)
                  & SPI_PRESC_gm)                         /* clock divider  */
               | (clk2x ? SPI_CLK2X_bm : 0x00U)          /* optional ×2    */
               | SPI_ENABLE_bm;                           /* enable last    */

    /* ---- 4. Initialise driver state -------------------------------------- */
    RingBuf_Init(&s_txBuf);
    RingBuf_Init(&s_rxBuf);
    s_txRemaining    = 0U;
    s_transferActive = false;
    s_timeoutCount   = 0U;
}

/* -------------------------------------------------------------------------- */
void SPI_Deinit(void)
{
    /* Disable the SPI interrupt before clearing ENABLE to prevent a
     * spurious ISR fire if a transfer was just finishing.                    */
    SPI0.INTCTRL  = 0x00U;
    SPI0.CTRLA   &= (uint8_t)~SPI_ENABLE_bm;
    s_transferActive = false;
}

/* =========================================================================
 * Public API – chip-select helpers
 * ========================================================================= */

void SPI_CS_Low(PORT_t *port, uint8_t pin_bm)
{
    /* Pull the CS pin low to assert the chip select.
     * The OUTCLR register sets the selected bit(s) to 0 atomically.         */
    port->OUTCLR = pin_bm;
}

/* -------------------------------------------------------------------------- */
void SPI_CS_High(PORT_t *port, uint8_t pin_bm)
{
    /* De-assert CS: drive the pin high.
     * OUTSET is used rather than a read-modify-write on OUT to avoid a
     * race condition if another ISR touches the same port.                   */
    port->OUTSET = pin_bm;
}

/* =========================================================================
 * Public API – blocking transfers
 * ========================================================================= */

SPI_Status_t SPI_TransferByte(uint8_t data, uint8_t *rxByte)
{
    /* A simple cycle-counter timeout avoids a hung bus locking the CPU.
     * At 4 MHz the counter starts at SPI_TIMEOUT_MS * CYCLES_PER_MS.
     * This is not cycle-accurate (compiler optimizations vary) but is
     * conservative enough for practical use.                                 */
    uint32_t timeout = (uint32_t)SPI_TIMEOUT_MS * (uint32_t)SPI_CYCLES_PER_MS;

    /* Write the byte to transmit.  The SPI shift register begins clocking
     * immediately because ENABLE is set in master mode.                      */
    SPI0.DATA = data;

    /* Wait for the transfer-complete flag (IF = 1 when byte done).          */
    while (!(SPI0.INTFLAGS & SPI_IF_bm))
    {
        if (--timeout == 0U)
        {
            s_timeoutCount++;           /* record the error for diagnostics  */
            return SPI_ERR_TIMEOUT;
        }
    }

    /* Reading DATA clears the IF flag (AVR-Dx datasheet §24.6.5).          */
    if (rxByte != NULL)
    {
        *rxByte = SPI0.DATA;
    }
    else
    {
        (void)SPI0.DATA;                /* dummy read to clear the flag      */
    }

    return SPI_OK;
}

/* -------------------------------------------------------------------------- */
SPI_Status_t SPI_TransferBuffer(const uint8_t *txBuf,
                                uint8_t       *rxBuf,
                                size_t         len)
{
    /* Guard: at least one direction must be valid, and len must be > 0.     */
    if ((txBuf == NULL) && (rxBuf == NULL))
    {
        return SPI_ERR_NULL;
    }
    if (len == 0U)
    {
        return SPI_ERR_SIZE;
    }

    for (size_t i = 0U; i < len; i++)
    {
        /* Use 0xFF as the dummy TX byte when performing a read-only burst.
         * Most SPI flash/sensor devices treat any value as a NOP clock.     */
        uint8_t  txByte = (txBuf != NULL) ? txBuf[i] : 0xFFU;
        uint8_t  rxByte = 0x00U;

        SPI_Status_t st = SPI_TransferByte(txByte, &rxByte);
        if (st != SPI_OK)
        {
            return st;                  /* propagate timeout immediately     */
        }

        if (rxBuf != NULL)
        {
            rxBuf[i] = rxByte;
        }
    }

    return SPI_OK;
}

/* =========================================================================
 * Public API – non-blocking (interrupt-driven) transfers
 * ========================================================================= */

SPI_Status_t SPI_StartTransfer(const uint8_t *txBuf, size_t len)
{
    /* Validate arguments before touching any hardware registers.            */
    if (txBuf == NULL)
    {
        return SPI_ERR_NULL;
    }
    if ((len == 0U) || (len > SPI_BUFFER_SIZE))
    {
        return SPI_ERR_SIZE;
    }

    /* Reject if a previous non-blocking transfer is still running.         */
    if (s_transferActive)
    {
        return SPI_ERR_BUSY;
    }

    /* ---- Load TX ring buffer and reset RX ring buffer -----------------   */
    RingBuf_Init(&s_txBuf);
    RingBuf_Init(&s_rxBuf);

    for (size_t i = 0U; i < len; i++)
    {
        RingBuf_Push(&s_txBuf, txBuf[i]);
    }

    /* ---- Arm driver state before enabling the interrupt ------------------
     * Setting s_transferActive BEFORE enabling the interrupt prevents a
     * scenario where the ISR fires and clears the flag before this function
     * returns, leaving s_transferActive stuck at false.                     */
    s_txRemaining    = len;
    s_transferActive = true;

    /* ---- Kick off the first byte -----------------------------------------
     * Writing DATA in master mode starts clocking immediately.  The
     * Transfer-Complete interrupt (IE bit) fires when the byte is done and
     * will advance the transfer from there.                                 */
    SPI0.DATA    = RingBuf_Pop(&s_txBuf);
    s_txRemaining--;

    /* Enable the Transfer-Complete interrupt (IE = SPI Interrupt Enable).  */
    SPI0.INTCTRL = SPI_IE_bm;

    return SPI_OK;
}

/* -------------------------------------------------------------------------- */
bool SPI_TransferComplete(void)
{
    /* s_transferActive is cleared by the ISR after the last byte.          */
    return !s_transferActive;
}

/* -------------------------------------------------------------------------- */
size_t SPI_ReadNonBlocking(uint8_t *rxBuf, size_t len)
{
    if (rxBuf == NULL)
    {
        return 0U;
    }

    size_t copied = 0U;

    while ((copied < len) && !RingBuf_IsEmpty(&s_rxBuf))
    {
        rxBuf[copied++] = RingBuf_Pop(&s_rxBuf);
    }

    return copied;
}

/* =========================================================================
 * Public API – diagnostics
 * ========================================================================= */

uint32_t SPI_GetTimeoutCount(void)
{
    return s_timeoutCount;
}

/* -------------------------------------------------------------------------- */
void SPI_ClearTimeoutCount(void)
{
    s_timeoutCount = 0U;
}

/* =========================================================================
 * ISR – SPI0 Transfer Complete
 * ========================================================================= */

/**
 * @brief  SPI0 transfer-complete interrupt service routine.
 *
 * Fires after every byte exchange when SPI0.INTCTRL.IE is set.
 *
 * Responsibilities:
 *   1. Read the received byte from SPI0.DATA (also clears IF flag).
 *   2. Push the received byte into the RX ring buffer.
 *   3. If more TX bytes remain, pop the next byte and write it to SPI0.DATA.
 *   4. If all bytes have been exchanged, disable the interrupt and clear
 *      s_transferActive so SPI_TransferComplete() returns true.
 *
 * @note  This ISR must run to completion before the SPI clock stretching
 *        deadline (depends on mode and clock speed).  Keep ISR code minimal.
 */
ISR(SPI0_INT_vect)
{
    /* Reading DATA clears the IF flag and must happen first to avoid a
     * re-entrant interrupt on some AVR-Dx silicon revisions.               */
    uint8_t received = SPI0.DATA;

    /* Save received byte into the RX ring buffer for later retrieval.      */
    if (!RingBuf_IsFull(&s_rxBuf))
    {
        RingBuf_Push(&s_rxBuf, received);
    }
    /* (Overflow silently discarded – caller must ensure len ≤ buffer size.) */

    /* Transmit the next byte if any remain.                                 */
    if (s_txRemaining > 0U)
    {
        SPI0.DATA = RingBuf_Pop(&s_txBuf);
        s_txRemaining--;
    }
    else
    {
        /* All bytes exchanged – disable the interrupt and signal completion. */
        SPI0.INTCTRL    = 0x00U;
        s_transferActive = false;
        /* Chip-select de-assertion is the caller's responsibility.          */
    }
}

/* =========================================================================
 * Private helpers – ring buffer
 * ========================================================================= */

/**
 * @brief  Initialise (reset) a ring buffer to the empty state.
 * @param  rb  Pointer to the ring buffer to reset.
 */
static inline void RingBuf_Init(RingBuffer_t *rb)
{
    rb->head = 0U;
    rb->tail = 0U;
}

/**
 * @brief  Return true if the ring buffer contains no data.
 * @param  rb  Pointer to the ring buffer.
 */
static inline bool RingBuf_IsEmpty(const RingBuffer_t *rb)
{
    return (rb->head == rb->tail);
}

/**
 * @brief  Return true if the ring buffer has no free space.
 * @param  rb  Pointer to the ring buffer.
 */
static inline bool RingBuf_IsFull(const RingBuffer_t *rb)
{
    return (((rb->head + 1U) & SPI_BUF_MASK) == rb->tail);
}

/**
 * @brief  Remove and return the oldest byte from the ring buffer.
 *
 * @pre    The buffer must not be empty; behaviour is undefined if it is.
 *         Always call RingBuf_IsEmpty() before calling this function.
 *
 * @param  rb  Pointer to the ring buffer.
 * @return     The next byte in FIFO order.
 */
static inline uint8_t RingBuf_Pop(RingBuffer_t *rb)
{
    uint8_t byte  = rb->buf[rb->tail];
    rb->tail      = (rb->tail + 1U) & SPI_BUF_MASK;
    return byte;
}

/**
 * @brief  Add a byte to the ring buffer.
 *
 * @pre    The buffer must not be full; check RingBuf_IsFull() first.
 *         If called on a full buffer the oldest byte is silently overwritten.
 *
 * @param  rb    Pointer to the ring buffer.
 * @param  byte  Byte to enqueue.
 */
static inline void RingBuf_Push(RingBuffer_t *rb, uint8_t byte)
{
    rb->buf[rb->head] = byte;
    rb->head          = (rb->head + 1U) & SPI_BUF_MASK;
}
