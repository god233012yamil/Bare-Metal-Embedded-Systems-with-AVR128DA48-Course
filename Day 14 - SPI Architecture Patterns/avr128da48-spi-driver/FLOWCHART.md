# SPI Driver — Flowcharts

**Target:** AVR128DA48 Curiosity Nano  
**Driver files:** `spi_driver.h` / `spi_driver.c`  
**Toolchain:** Atmel / Microchip Studio 7 — GCC C Executable Project  

---

## Table of Contents

1. [System-Level Overview](#1-system-level-overview)
2. [SPI_Init](#2-spi_init)
3. [SPI_Deinit](#3-spi_deinit)
4. [Chip-Select Helpers — SPI_CS_Low / SPI_CS_High](#4-chip-select-helpers)
5. [Blocking Transfer — SPI_TransferByte](#5-blocking-transfer--spi_transferbyte)
6. [Blocking Transfer — SPI_TransferBuffer](#6-blocking-transfer--spi_transferbuffer)
7. [Non-Blocking Transfer — SPI_StartTransfer](#7-non-blocking-transfer--spi_starttransfer)
8. [Non-Blocking Query — SPI_TransferComplete](#8-non-blocking-query--spi_transfercomplete)
9. [Non-Blocking Read — SPI_ReadNonBlocking](#9-non-blocking-read--spi_readnonblocking)
10. [ISR — SPI0_INT_vect (Transfer-Complete Interrupt)](#10-isr--spi0_int_vect)
11. [Ring Buffer Operations](#11-ring-buffer-operations)
12. [Diagnostics — SPI_GetTimeoutCount / SPI_ClearTimeoutCount](#12-diagnostics)
13. [Complete Non-Blocking Call Sequence (Caller + Driver + ISR)](#13-complete-non-blocking-call-sequence)

---

## 1. System-Level Overview

High-level picture of the two transfer modes and how they relate to the
hardware, the ISR, and the application layer.

```mermaid
flowchart TD
    APP(["Application / Sensor Driver"])

    subgraph BLOCKING ["Blocking Mode  (CPU waits)"]
        direction TB
        TB["SPI_TransferByte()"]
        TBuf["SPI_TransferBuffer()"]
        TBuf -->|"calls per byte"| TB
    end

    subgraph NONBLOCKING ["Non-Blocking Mode  (ISR-driven)"]
        direction TB
        ST["SPI_StartTransfer()"]
        ISR(["ISR — SPI0_INT_vect"])
        TC["SPI_TransferComplete()"]
        RNB["SPI_ReadNonBlocking()"]
        ST -->|"kicks first byte\nenable INT"| ISR
        ISR -->|"sets s_transferActive=false\nwhen done"| TC
        TC -->|"true → caller reads"| RNB
    end

    CS["SPI_CS_Low() / SPI_CS_High()"]
    HW[["AVR128DA48\nSPI0 Hardware\nPC0=SCK  PC1=MISO  PC2=MOSI"]]

    APP -->|"assert CS"| CS
    APP --> BLOCKING
    APP --> NONBLOCKING
    CS --> HW
    TB -->|"SPI0.DATA read/write\npolls INTFLAGS.IF"| HW
    ISR <-->|"SPI0.DATA read/write\nINTFLAGS.IF cleared on read"| HW

    style APP fill:#4A90D9,color:#fff
    style HW fill:#E67E22,color:#fff
    style ISR fill:#8E44AD,color:#fff
    style BLOCKING fill:#EAF4FB,stroke:#4A90D9
    style NONBLOCKING fill:#F5EEF8,stroke:#8E44AD
```

---

## 2. SPI_Init

Configures PORTC pin directions, writes `SPI0.CTRLB` (mode + SSD),
writes `SPI0.CTRLA` (master + prescaler + CLK2X + enable), then
resets all driver state variables.

```mermaid
flowchart TD
    A([Enter SPI_Init\nmode, prescaler, clk2x]) --> B

    B["PORTC.DIRSET = PC0 SCK + PC2 MOSI\nPORTC.DIRCLR = PC1 MISO"]
    B --> C

    C["SPI0.CTRLB =\n  SPI_SSD_bm  ← disable HW SS\n  MODE bits    ← CPOL/CPHA"]
    C --> D

    D{"clk2x == true?"}
    D -->|Yes| E["CTRLA |= SPI_CLK2X_bm"]
    D -->|No| F["CLK2X bit = 0"]
    E --> G
    F --> G

    G["SPI0.CTRLA =\n  SPI_MASTER_bm\n  PRESC[1:0] from prescaler\n  CLK2X (if set)\n  SPI_ENABLE_bm  ← written last"]
    G --> H

    H["RingBuf_Init(s_txBuf)\nRingBuf_Init(s_rxBuf)\ns_txRemaining    = 0\ns_transferActive = false\ns_timeoutCount   = 0"]
    H --> Z([Return])

    style A fill:#4A90D9,color:#fff
    style Z fill:#4A90D9,color:#fff
    style G fill:#F9EBEA,stroke:#C0392B
```

---

## 3. SPI_Deinit

Safe shutdown sequence — interrupt disabled before ENABLE cleared to
prevent a spurious ISR fire at the end of a transfer.

```mermaid
flowchart TD
    A([Enter SPI_Deinit]) --> B
    B["SPI0.INTCTRL = 0x00\n← disable Transfer-Complete interrupt first"]
    B --> C
    C["SPI0.CTRLA &= ~SPI_ENABLE_bm\n← disable SPI peripheral"]
    C --> D
    D["s_transferActive = false\n← clear driver busy flag"]
    D --> Z([Return])

    style A fill:#4A90D9,color:#fff
    style Z fill:#4A90D9,color:#fff
    style B fill:#FDFEFE,stroke:#E74C3C
```

---

## 4. Chip-Select Helpers

`SPI_CS_Low` and `SPI_CS_High` are thin wrappers around atomic
PORT register writes.  Both follow the same shape.

```mermaid
flowchart LR
    subgraph LOW ["SPI_CS_Low(port, pin_bm)"]
        direction TB
        L1([Enter]) --> L2["port→OUTCLR = pin_bm\n← atomic bit-clear\nCS driven LOW  active"]
        L2 --> L3([Return])
    end

    subgraph HIGH ["SPI_CS_High(port, pin_bm)"]
        direction TB
        H1([Enter]) --> H2["port→OUTSET = pin_bm\n← atomic bit-set\nCS driven HIGH  idle"]
        H2 --> H3([Return])
    end

    style LOW  fill:#EAF9EA,stroke:#27AE60
    style HIGH fill:#FDFEFE,stroke:#BDC3C7
```

> **Why OUTCLR / OUTSET instead of OUT?**  
> These registers perform a single atomic write to one bit without a
> read-modify-write cycle, eliminating the race condition that would occur
> if an ISR modified another pin on the same port between the read and write.

---

## 5. Blocking Transfer — SPI_TransferByte

The fundamental blocking primitive.  Every blocking call in the driver
ultimately passes through here.

```mermaid
flowchart TD
    A([Enter SPI_TransferByte\ndata, rxByte ptr]) --> B

    B["timeout =\nSPI_TIMEOUT_MS × SPI_CYCLES_PER_MS\n= 100 × 4000 = 400 000 cycles"]
    B --> C

    C["SPI0.DATA = data\n← write starts SPI clock immediately"]
    C --> D

    D{"SPI0.INTFLAGS\n& SPI_IF_bm ?"}
    D -->|"IF = 1\nbyte done"| G
    D -->|"IF = 0\nstill shifting"| E

    E{"timeout-- == 0 ?"}
    E -->|No, still waiting| D
    E -->|"Yes — bus stall"| F

    F["s_timeoutCount++"]
    F --> FE(["Return SPI_ERR_TIMEOUT"])

    G{"rxByte != NULL ?"}
    G -->|Yes| H["*rxByte = SPI0.DATA\n← read clears IF flag"]
    G -->|No| I["(void) SPI0.DATA\n← dummy read to clear IF"]
    H --> Z
    I --> Z

    Z(["Return SPI_OK"])

    style A fill:#4A90D9,color:#fff
    style Z fill:#27AE60,color:#fff
    style FE fill:#E74C3C,color:#fff
    style D fill:#FEF9E7,stroke:#F39C12
    style E fill:#FEF9E7,stroke:#F39C12
```

---

## 6. Blocking Transfer — SPI_TransferBuffer

Iterates over a byte array, calling `SPI_TransferByte` for each element.
Either TX or RX buffer (but not both) may be `NULL`.

```mermaid
flowchart TD
    A([Enter SPI_TransferBuffer\ntxBuf, rxBuf, len]) --> G1

    G1{"txBuf == NULL\nAND rxBuf == NULL ?"}
    G1 -->|Yes| E1(["Return SPI_ERR_NULL"])
    G1 -->|No| G2

    G2{"len == 0 ?"}
    G2 -->|Yes| E2(["Return SPI_ERR_SIZE"])
    G2 -->|No| LOOP

    LOOP["i = 0"]
    LOOP --> CHK

    CHK{"i < len ?"}
    CHK -->|No — done| OK(["Return SPI_OK"])
    CHK -->|Yes| TX

    TX{"txBuf != NULL ?"}
    TX -->|Yes| T1["txByte = txBuf[i]"]
    TX -->|No| T2["txByte = 0xFF\n← dummy byte for read-only"]
    T1 --> CALL
    T2 --> CALL

    CALL["st = SPI_TransferByte(txByte, &rxByte)"]
    CALL --> STCHK

    STCHK{"st != SPI_OK ?"}
    STCHK -->|"Yes (timeout)"| PROP(["Return st\n← propagate immediately"])
    STCHK -->|No| RXCHK

    RXCHK{"rxBuf != NULL ?"}
    RXCHK -->|Yes| SAVE["rxBuf[i] = rxByte"]
    RXCHK -->|No| NEXT["discard rxByte"]
    SAVE --> INC
    NEXT --> INC

    INC["i++"]
    INC --> CHK

    style A   fill:#4A90D9,color:#fff
    style OK  fill:#27AE60,color:#fff
    style E1  fill:#E74C3C,color:#fff
    style E2  fill:#E74C3C,color:#fff
    style PROP fill:#E74C3C,color:#fff
    style G1  fill:#FEF9E7,stroke:#F39C12
    style G2  fill:#FEF9E7,stroke:#F39C12
    style STCHK fill:#FEF9E7,stroke:#F39C12
```

---

## 7. Non-Blocking Transfer — SPI_StartTransfer

Validates arguments, loads the TX ring buffer, arms state variables,
writes the first byte to hardware, then enables the ISR and returns
immediately to the caller.

```mermaid
flowchart TD
    A([Enter SPI_StartTransfer\ntxBuf, len]) --> V1

    V1{"txBuf == NULL ?"}
    V1 -->|Yes| E1(["Return SPI_ERR_NULL"])
    V1 -->|No| V2

    V2{"len == 0\nOR len > SPI_BUFFER_SIZE ?"}
    V2 -->|Yes| E2(["Return SPI_ERR_SIZE"])
    V2 -->|No| V3

    V3{"s_transferActive\n== true ?"}
    V3 -->|Yes| E3(["Return SPI_ERR_BUSY"])
    V3 -->|No| INIT

    INIT["RingBuf_Init(s_txBuf)\nRingBuf_Init(s_rxBuf)\n← reset both ring buffers"]
    INIT --> FILL

    FILL["for i in 0…len-1\n  RingBuf_Push(s_txBuf, txBuf[i])\n← copy all bytes into TX ring"]
    FILL --> ARM

    ARM["s_txRemaining    = len\ns_transferActive = true\n← arm state BEFORE enabling INT\n  (prevents missed-flag race)"]
    ARM --> KICK

    KICK["SPI0.DATA = RingBuf_Pop(s_txBuf)\ns_txRemaining--\n← write byte 0 → SPI clock starts"]
    KICK --> INT

    INT["SPI0.INTCTRL = SPI_IE_bm\n← enable Transfer-Complete interrupt"]
    INT --> Z(["Return SPI_OK\n← ISR takes over from here"])

    style A  fill:#4A90D9,color:#fff
    style Z  fill:#27AE60,color:#fff
    style E1 fill:#E74C3C,color:#fff
    style E2 fill:#E74C3C,color:#fff
    style E3 fill:#E74C3C,color:#fff
    style ARM fill:#F5EEF8,stroke:#8E44AD
    style KICK fill:#F5EEF8,stroke:#8E44AD
    style INT fill:#F5EEF8,stroke:#8E44AD
    style V1 fill:#FEF9E7,stroke:#F39C12
    style V2 fill:#FEF9E7,stroke:#F39C12
    style V3 fill:#FEF9E7,stroke:#F39C12
```

---

## 8. Non-Blocking Query — SPI_TransferComplete

A single-expression poll of the `s_transferActive` flag set/cleared by
the ISR.  No side-effects.

```mermaid
flowchart TD
    A([Enter SPI_TransferComplete]) --> B

    B{"s_transferActive\n== true ?"}
    B -->|"Yes — ISR still running"| NO(["Return false"])
    B -->|"No  — ISR finished"| YES(["Return true"])

    style A   fill:#4A90D9,color:#fff
    style YES fill:#27AE60,color:#fff
    style NO  fill:#E67E22,color:#fff
    style B   fill:#FEF9E7,stroke:#F39C12
```

---

## 9. Non-Blocking Read — SPI_ReadNonBlocking

Drains the internal RX ring buffer into the caller's buffer after
`SPI_TransferComplete()` has returned `true`.

```mermaid
flowchart TD
    A([Enter SPI_ReadNonBlocking\nrxBuf, len]) --> V1

    V1{"rxBuf == NULL ?"}
    V1 -->|Yes| E(["Return 0"])
    V1 -->|No| INIT

    INIT["copied = 0"]
    INIT --> LOOP

    LOOP{"copied < len\nAND RingBuf not empty ?"}
    LOOP -->|No| Z(["Return copied"])
    LOOP -->|Yes| POP

    POP["rxBuf[copied] = RingBuf_Pop(s_rxBuf)\ncopied++"]
    POP --> LOOP

    style A    fill:#4A90D9,color:#fff
    style Z    fill:#27AE60,color:#fff
    style E    fill:#E74C3C,color:#fff
    style LOOP fill:#FEF9E7,stroke:#F39C12
```

---

## 10. ISR — SPI0_INT_vect

Fires automatically after each byte exchange.  Keeps its own logic minimal
to stay within the inter-byte deadline.

```mermaid
flowchart TD
    A(["SPI0_INT_vect fires\n← hardware: 8 bits shifted"]) --> RD

    RD["received = SPI0.DATA\n← read FIRST: clears IF flag\n  prevents re-entrant interrupt"]
    RD --> FULL

    FULL{"RingBuf_IsFull\n(s_rxBuf) ?"}
    FULL -->|"No — space available"| PUSH
    FULL -->|"Yes — overflow"| MORE
    PUSH["RingBuf_Push(s_rxBuf, received)\n← store received byte"]
    PUSH --> MORE

    MORE{"s_txRemaining > 0 ?"}
    MORE -->|"Yes — more to send"| SEND
    MORE -->|"No  — all done"| DONE

    SEND["SPI0.DATA = RingBuf_Pop(s_txBuf)\ns_txRemaining--\n← next byte → SPI clock restarts"]
    SEND --> Z(["Return from ISR\n← next byte clocking"])

    DONE["SPI0.INTCTRL = 0x00\n← disable this interrupt\ns_transferActive = false\n← signal completion to caller"]
    DONE --> Z2(["Return from ISR\n← transfer complete"])

    style A    fill:#8E44AD,color:#fff
    style Z    fill:#8E44AD,color:#fff
    style Z2   fill:#27AE60,color:#fff
    style RD   fill:#F5EEF8,stroke:#8E44AD
    style FULL fill:#FEF9E7,stroke:#F39C12
    style MORE fill:#FEF9E7,stroke:#F39C12
    style DONE fill:#EAF9EA,stroke:#27AE60
```

> **Critical ordering:** `SPI0.DATA` must be read **before** any other
> register access.  Reading DATA clears the `INTFLAGS.IF` bit, which
> prevents a spurious re-entry on some AVR-Dx silicon revisions.

---

## 11. Ring Buffer Operations

All four operations share the same `head`/`tail` index scheme with
bitmask wrap-around (`index & SPI_BUF_MASK`).

```mermaid
flowchart TD
    subgraph INIT_RB ["RingBuf_Init(rb)"]
        I1([Enter]) --> I2["rb→head = 0\nrb→tail = 0"]
        I2 --> I3([Return])
    end

    subgraph EMPTY ["RingBuf_IsEmpty(rb)"]
        EM1([Enter]) --> EM2{"head == tail ?"}
        EM2 -->|Yes| EM3(["Return true"])
        EM2 -->|No| EM4(["Return false"])
    end

    subgraph FULL_RB ["RingBuf_IsFull(rb)"]
        F1([Enter]) --> F2{"(head+1) & MASK\n== tail ?"}
        F2 -->|Yes| F3(["Return true"])
        F2 -->|No| F4(["Return false"])
    end

    subgraph POP_RB ["RingBuf_Pop(rb)"]
        P1([Enter]) --> P2["byte = buf[tail]"]
        P2 --> P3["tail = (tail+1) & MASK"]
        P3 --> P4(["Return byte"])
    end

    subgraph PUSH_RB ["RingBuf_Push(rb, byte)"]
        PU1([Enter]) --> PU2["buf[head] = byte"]
        PU2 --> PU3["head = (head+1) & MASK"]
        PU3 --> PU4([Return])
    end

    style INIT_RB  fill:#EAF4FB,stroke:#4A90D9
    style EMPTY    fill:#EAF9EA,stroke:#27AE60
    style FULL_RB  fill:#FDFEFE,stroke:#BDC3C7
    style POP_RB   fill:#FEF9E7,stroke:#F39C12
    style PUSH_RB  fill:#F5EEF8,stroke:#8E44AD
```

**Buffer state diagram:**

```mermaid
stateDiagram-v2
    [*] --> Empty : RingBuf_Init

    Empty --> Partial    : Push (head advances)
    Partial --> Partial  : Push or Pop
    Partial --> Full     : Push until (head+1)&mask == tail
    Partial --> Empty    : Pop until head == tail
    Full --> Partial     : Pop (tail advances)
    Full --> Full        : Push silently overwrites\n(caller must avoid this)
```

---

## 12. Diagnostics

```mermaid
flowchart LR
    subgraph GET ["SPI_GetTimeoutCount()"]
        direction TB
        G1([Enter]) --> G2(["Return s_timeoutCount\n← read-only, no side effects"])
    end

    subgraph CLR ["SPI_ClearTimeoutCount()"]
        direction TB
        C1([Enter]) --> C2["s_timeoutCount = 0"]
        C2 --> C3([Return])
    end

    subgraph INC_TC ["Incremented by SPI_TransferByte()"]
        direction TB
        T1(["timeout counter\nreaches 0"]) --> T2["s_timeoutCount++\nthen return SPI_ERR_TIMEOUT"]
    end

    INC_TC -->|"increments"| GET

    style GET    fill:#EAF4FB,stroke:#4A90D9
    style CLR    fill:#FEF9E7,stroke:#F39C12
    style INC_TC fill:#FDFEFE,stroke:#E74C3C
```

---

## 13. Complete Non-Blocking Call Sequence

End-to-end sequence showing the interplay between the **caller** (application
or sensor driver), the **SPI driver API**, and the **hardware ISR**.
This is the flow demonstrated by `demo_non_blocking()` in `main.c`.

```mermaid
sequenceDiagram
    participant APP  as Caller (main.c)
    participant DRV  as SPI Driver API
    participant HW   as SPI0 Hardware
    participant ISR  as SPI0_INT_vect

    APP  ->>  DRV  : SPI_Init(MODE_0, DIV4, false)
    DRV  ->>  HW   : configure PORTC, CTRLB, CTRLA

    APP  ->>  DRV  : sei()  — enable global interrupts

    APP  ->>  DRV  : SPI_CS_Low(&PORTA, PIN4_bm)
    DRV  ->>  HW   : PORTA.OUTCLR = PIN4_bm  [CS asserted]

    APP  ->>  DRV  : SPI_StartTransfer(txBuf, len)
    DRV  ->>  DRV  : validate args, load TX ring buffer
    DRV  ->>  DRV  : s_transferActive = true
    DRV  ->>  HW   : SPI0.DATA = byte[0]  [first byte starts clocking]
    DRV  ->>  HW   : SPI0.INTCTRL = SPI_IE_bm  [enable ISR]
    DRV  -->> APP  : return SPI_OK  ← immediately

    loop CPU free — other work runs here
        APP  ->>  DRV  : SPI_TransferComplete() ?
        DRV  -->> APP  : false  (s_transferActive == true)

        Note over HW,ISR: SPI clock running autonomously

        HW   ->>  ISR  : INTFLAGS.IF set — byte N done
        ISR  ->>  HW   : received = SPI0.DATA  [clears IF]
        ISR  ->>  DRV  : RingBuf_Push(s_rxBuf, received)
        alt more bytes remain
            ISR  ->>  HW  : SPI0.DATA = next byte  [restarts clock]
        else last byte done
            ISR  ->>  HW  : SPI0.INTCTRL = 0  [disable ISR]
            ISR  ->>  DRV : s_transferActive = false
        end
    end

    APP  ->>  DRV  : SPI_TransferComplete() ?
    DRV  -->> APP  : true  (s_transferActive == false)

    APP  ->>  DRV  : SPI_CS_High(&PORTA, PIN4_bm)
    DRV  ->>  HW   : PORTA.OUTSET = PIN4_bm  [CS released]

    APP  ->>  DRV  : SPI_ReadNonBlocking(rxBuf, len)
    DRV  ->>  DRV  : drain s_rxBuf → rxBuf
    DRV  -->> APP  : return bytes_copied
```

---

*Generated for AVR128DA48 — AVR-Dx Device Pack 2.4.286*
