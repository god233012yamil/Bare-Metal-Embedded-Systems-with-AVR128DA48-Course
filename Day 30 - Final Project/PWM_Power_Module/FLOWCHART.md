# System Flowcharts

This document contains Mermaid diagrams that describe the runtime behaviour of the PWM-Controlled Power Module firmware. GitHub renders Mermaid natively inside Markdown code fences tagged ` ```mermaid `.

---

## Table of Contents

1. [System Initialisation Sequence](#1-system-initialisation-sequence)
2. [Super-Loop (Main Loop)](#2-super-loop-main-loop)
3. [Cooperative Scheduler](#3-cooperative-scheduler)
4. [Control-Loop State Machine](#4-control-loop-state-machine)
5. [ADC Driver — Interrupt Flow](#5-adc-driver--interrupt-flow)
6. [USART Driver — Interrupt Flow](#6-usart-driver--interrupt-flow)
7. [AC Driver — Interrupt Flow](#7-ac-driver--interrupt-flow)
8. [EVSYS Autonomous Event Path](#8-evsys-autonomous-event-path)
9. [FIFO Ring Buffer — Put and Get](#9-fifo-ring-buffer--put-and-get)
10. [Task Timing Diagram](#10-task-timing-diagram)
11. [Full Peripheral Interaction Map](#11-full-peripheral-interaction-map)

---

## 1. System Initialisation Sequence

```mermaid
flowchart TD
    RESET([Power-On / Reset]) --> CLK

    CLK["Configure CPU clock
    CCP write to OSCHF at 24 MHz
    Wait for SOSC flag to clear"]
    CLK --> LED_INIT

    LED_INIT["Configure LED pin
    PC6 output, drive HIGH
    active-low LED off"]
    LED_INIT --> USART_INIT

    USART_INIT["USART_Init()
    PORTMUX to PA0/PA1
    PA0 output TXD
    Set BAUD register
    Enable TX and RX
    Enable RXCIE interrupt
    Init RX FIFO"]
    USART_INIT --> ADC_INIT

    ADC_INIT["ADC_Init()
    PD2 input-disable analogue
    VREF.ADC0REF = 2.048 V
    ADC0 12-bit DIV16 prescaler
    MUXPOS = AIN2
    Enable STARTEI event trigger
    Enable RESRDY interrupt"]
    ADC_INIT --> AC_INIT

    AC_INIT["AC_Init()
    VREF.ACREF = VDD
    AC0 MUXCTRL AINP0 / DACREF
    AC0 DACREF = 128, 50% VDD
    INTCTRL BOTHEDGE interrupt
    CTRLA small hysteresis enable"]
    AC_INIT --> TCA_INIT

    TCA_INIT["TCA_Init()
    PORTMUX TCA0 to PORTD
    PD0 output
    TCA0 SINGLE mode
    SINGLESLOPE PWM CMP0EN
    PER = 1023, CMP0 = 0
    CTRLA DIV1 ENABLE"]
    TCA_INIT --> EVSYS_INIT

    EVSYS_INIT["EVSYS_Init()
    CHANNEL0 = AC0_OUT
    USERADC0START = CH0 0x01"]
    EVSYS_INIT --> SCHED_INIT

    SCHED_INIT["SCHED_Init()
    Clear task table
    RTC.CLKSEL = OSC32K
    RTC.PITCTRLA CYC32 PITEN
    RTC.PITINTCTRL PI enable
    tick approx 1024 Hz"]
    SCHED_INIT --> REG_TASKS

    REG_TASKS["Register scheduler tasks
    Task_ADC      @ 51 ticks
    Task_Control  @ 102 ticks
    Task_Telem    @ 512 ticks
    Task_LED      @ 512 ticks"]
    REG_TASKS --> SEI

    SEI(["sei() — Enable global interrupts"])
    SEI --> BANNER

    BANNER["USART_SendString()
    PWM Power Module v1.0
    header line"]
    BANNER --> SUPERLOOP([Enter super-loop])
```

---

## 2. Super-Loop (Main Loop)

```mermaid
flowchart TD
    START([Enter while loop]) --> SCHED

    SCHED["SCHED_Run()
    Check tick flag
    Dispatch due tasks"]
    SCHED --> RX_CHECK

    RX_CHECK{"USART_RxAvailable?"}
    RX_CHECK -- Yes --> GET_BYTE
    RX_CHECK -- No  --> SCHED

    GET_BYTE["USART_GetByte b
    Echo byte back
    USART_SendByte b"]
    GET_BYTE --> RX_CHECK
```

---

## 3. Cooperative Scheduler

```mermaid
flowchart TD
    PIT_ISR(["ISR RTC_PIT_vect"])
    PIT_ISR --> CLR_FLAG["RTC.PITINTFLAGS = PI_bm
    clear hardware flag"]
    CLR_FLAG --> SET_TICK["g_tickFlag = true
    g_tickCount++"]
    SET_TICK --> RETI([RETI])

    SCHED_RUN(["SCHED_Run() called from super-loop"])
    SCHED_RUN --> ATOMIC["cli()
    tick = g_tickFlag
    g_tickFlag = false
    sei()"]
    ATOMIC --> TICK_Q{"tick == true?"}
    TICK_Q -- No  --> RETURN([Return immediately])
    TICK_Q -- Yes --> LOOP_INIT["i = 0"]

    LOOP_INIT --> LOOP_CHK{"i < taskCount?"}
    LOOP_CHK -- No  --> RETURN
    LOOP_CHK -- Yes --> ENABLED{"task enabled?"}
    ENABLED -- No  --> NEXT
    ENABLED -- Yes --> DEC["task counter decrement"]
    DEC --> ZERO{"counter == 0?"}
    ZERO -- No  --> NEXT
    ZERO -- Yes --> RELOAD["task counter = task period"]
    RELOAD --> DISPATCH["task func()"]
    DISPATCH --> NEXT["i++"]
    NEXT --> LOOP_CHK
```

---

## 4. Control-Loop State Machine

```mermaid
stateDiagram-v2
    [*] --> IDLE : Reset

    IDLE : IDLE state 0
    SAMPLING : SAMPLING state 1
    ADJUSTING : ADJUSTING state 2
    RUNNING : RUNNING state 3

    IDLE --> SAMPLING : first Task_Control invocation\nEnable PWM output, clear ADC flag

    SAMPLING --> SAMPLING : ADC result not ready yet
    SAMPLING --> ADJUSTING : ADC_IsResultReady is true

    ADJUSTING --> RUNNING : duty = adc right-shift 2\nTCA_SetDuty applied\nMaps 0-4095 ADC to 0-1023 PWM

    RUNNING --> SAMPLING : no AC event, normal loop
    RUNNING --> RUNNING : AC fires HIGH, clamp duty to MAX, log event
    RUNNING --> SAMPLING : AC fires LOW, resume normal control
```

---

## 5. ADC Driver — Interrupt Flow

```mermaid
flowchart TD
    TRIGGER(["Trigger source:
    Task_ADC calls ADC_StartConversion
    writes STCONV bit
    OR EVSYS CH0 from AC0 event
    hardware no CPU"])
    TRIGGER --> CONV["ADC0 samples AIN2 on PD2
    12-bit conversion in progress
    approx 1 us at 1.5 MHz ADC clock"]
    CONV --> RESRDY_ISR(["ISR ADC0_RESRDY_vect"])
    RESRDY_ISR --> READ_RES["g_adcResult = ADC0.RES
    reading RES clears RESRDY flag"]
    READ_RES --> SET_FLAG["g_resultReady = true"]
    SET_FLAG --> RETI([RETI])

    TASK["Task_Control()
    in SAMPLING state"]
    TASK --> POLL{"ADC_IsResultReady?"}
    POLL -- No  --> RETURN_TASK([Stay in SAMPLING])
    POLL -- Yes --> TRANSITION([Transition to ADJUSTING])
```

---

## 6. USART Driver — Interrupt Flow

```mermaid
flowchart TD
    UART_RX(["UART byte arrives on PA1"])
    UART_RX --> RXC_ISR(["ISR USART0_RXC_vect"])
    RXC_ISR --> READ_DATA["data = USART0.RXDATAL
    reading clears RXCIF"]
    READ_DATA --> FIFO_PUT["FIFO_PutByte to g_rxFifo"]
    FIFO_PUT --> FULL{"FIFO full?"}
    FULL -- No  --> STORE["Store byte at buf head
    advance head with mask"]
    FULL -- Yes --> DROP["Byte discarded
    overflow"]
    STORE --> RETI2([RETI])
    DROP  --> RETI2

    SUPERLOOP["Super-loop
    USART_RxAvailable?"]
    SUPERLOOP --> AVAIL{"head != tail?"}
    AVAIL -- No  --> IDLE2([Continue loop])
    AVAIL -- Yes --> GET["USART_GetByte
    read buf at tail
    advance tail with mask"]
    GET --> ECHO["USART_SendByte b
    polled TX waits DREIF"]
    ECHO --> SUPERLOOP

    TX_TELEM(["Task_Telemetry()
    USART_SendString / SendUInt16"])
    TX_TELEM --> WAIT_DREIF{"USART0.STATUS DREIF set?"}
    WAIT_DREIF -- No  --> WAIT_DREIF
    WAIT_DREIF -- Yes --> WRITE_TX["USART0.TXDATAL = byte"]
    WRITE_TX --> MORE{"More bytes?"}
    MORE -- Yes --> WAIT_DREIF
    MORE -- No  --> DONE_TX([Done])
```

---

## 7. AC Driver — Interrupt Flow

```mermaid
flowchart TD
    SIGNAL(["Analogue signal on PD2
    crosses DACREF threshold"])
    SIGNAL --> AC_FIRES["AC0 output toggles
    CMPSTATE bit changes"]
    AC_FIRES --> AC_ISR(["ISR AC0_AC_vect"])
    AC_ISR --> CLR_HW["AC0.STATUS = CMPIF_bm
    write-1-to-clear hardware flag"]
    CLR_HW --> SET_SW["g_acFlag = true"]
    SET_SW --> RETI3([RETI])

    CTRL_TASK["Task_Control()
    in RUNNING state"]
    CTRL_TASK --> CHK_FLAG{"AC_IsFlagSet?"}
    CHK_FLAG -- No  --> LOOP_BACK([Back to SAMPLING])
    CHK_FLAG -- Yes --> CLR_SW["AC_ClearFlag()
    g_acFlag = false"]
    CLR_SW --> CHK_OUT{"AC_GetOutput?
    CMPSTATE bit"}
    CHK_OUT -- HIGH --> CLAMP["TCA_SetDuty MAX
    Log Threshold HIGH"]
    CHK_OUT -- LOW  --> RESUME["Log Threshold LOW
    Resume normal control"]
    CLAMP  --> LOOP_BACK
    RESUME --> LOOP_BACK
```

---

## 8. EVSYS Autonomous Event Path

```mermaid
flowchart LR
    AC0_OUT(["AC0 output edge
    CMPSTATE toggles"])
    CH0(["EVSYS CHANNEL0
    generator = AC0_OUT_gc"])
    ADC_START(["ADC0 START
    USERADC0START = 0x01"])
    CONV2(["ADC0 begins conversion
    automatic zero CPU latency"])
    ISR2(["ADC0_RESRDY_vect fires
    g_resultReady = true"])

    AC0_OUT -->|hardware event| CH0
    CH0     -->|channel routing| ADC_START
    ADC_START -->|EVCTRL.STARTEI| CONV2
    CONV2   -->|RESRDY interrupt| ISR2

    style AC0_OUT   fill:#f9a,stroke:#c00
    style CH0       fill:#adf,stroke:#06c
    style ADC_START fill:#adf,stroke:#06c
    style CONV2     fill:#afa,stroke:#060
    style ISR2      fill:#afa,stroke:#060
```

> This path operates entirely in hardware. The CPU is not involved until the RESRDY ISR fires after the conversion completes.

---

## 9. FIFO Ring Buffer — Put and Get

```mermaid
flowchart TD
    subgraph PUT ["FIFO_PutByte — Producer ISR context"]
        P1(["Called with byte"])
        P2["nextHead = head + 1 masked"]
        P3{"nextHead == tail?
        buffer full"}
        P4["buf at head = byte
        head = nextHead"]
        P5(["return true"])
        P6(["return false
        byte dropped"])
        P1 --> P2 --> P3
        P3 -- No  --> P4 --> P5
        P3 -- Yes --> P6
    end

    subgraph GET ["FIFO_GetByte — Consumer main context"]
        G1(["Called with byte pointer"])
        G2{"head == tail?
        buffer empty"}
        G3["byte = buf at tail
        tail = tail + 1 masked"]
        G4(["return true"])
        G5(["return false"])
        G1 --> G2
        G2 -- No  --> G3 --> G4
        G2 -- Yes --> G5
    end
```

---

## 10. Task Timing Diagram

```mermaid
gantt
    title Scheduler Task Periods (relative to RTC PIT tick, approx 1 ms per unit)
    dateFormat X
    axisFormat %s

    section RTC PIT
    Tick 0               :milestone, m0, 0,   1
    Tick 51              :milestone, m1, 51,  52
    Tick 102             :milestone, m2, 102, 103
    Tick 512             :milestone, m3, 512, 513

    section Task_ADC 50 ms
    ADC Task             :active, t1, 0,   51
    ADC Task             :active, t2, 51,  102
    ADC Task             :active, t3, 102, 153

    section Task_Control 100 ms
    Control Task         :crit, c1, 0,   102
    Control Task         :crit, c2, 102, 204
    Control Task         :crit, c3, 204, 306

    section Task_Telemetry 500 ms
    Telemetry Task       :done, tl1, 0,   512
    Telemetry Task       :done, tl2, 512, 1024

    section Task_LED 500 ms
    LED Toggle           :done, l1, 0,   512
    LED Toggle           :done, l2, 512, 1024
```

---

## 11. Full Peripheral Interaction Map

```mermaid
graph TD
    subgraph MCU ["AVR128DA48"]
        subgraph CLK ["Clock Domain"]
            OSCHF["OSCHF 24 MHz
            CPU and peripherals"]
            OSC32K["OSC32K 32.768 kHz
            RTC PIT"]
        end

        subgraph APP ["Application Layer main.c"]
            FSM["Control FSM
            4 states"]
            TASKS["Scheduler Tasks
            ADC / Control / Telem / LED"]
        end

        subgraph SCHED_MOD ["scheduler.c"]
            PIT["RTC PIT ISR
            approx 1024 Hz"]
            DISPATCH["Task Dispatcher"]
        end

        subgraph ADC_MOD ["adc.c"]
            ADC0["ADC0
            12-bit AIN2"]
            RESRDY["RESRDY ISR"]
        end

        subgraph TCA_MOD ["tca.c"]
            TCA0["TCA0 Single
            SingleSlope PWM"]
        end

        subgraph AC_MOD ["ac.c"]
            AC0["AC0
            DAC REF threshold"]
            AC_ISR["AC ISR
            BOTHEDGE"]
        end

        subgraph USART_MOD ["usart.c"]
            USART0["USART0
            115200 8N1"]
            RXC_ISR["RXC ISR"]
            FIFO_MOD["RX FIFO
            64 bytes"]
        end

        subgraph EVSYS_MOD ["evsys.c"]
            CH0_EV["EVSYS CH0
            AC0 OUT to ADC0 START"]
        end

        subgraph VREF_MOD ["VREF block"]
            VREF_ADC["ADC0 REF
            2.048 V"]
            VREF_AC["AC REF
            VDD"]
        end
    end

    subgraph BOARD ["Curiosity Nano Board"]
        POT["Potentiometer
        or signal source PD2"]
        PWM_OUT["PWM Output
        PD0"]
        LED_HW["LED active-low
        PC6"]
        USB["CDC-USB
        PA0 / PA1"]
    end

    OSC32K --> PIT
    OSCHF  --> ADC0
    OSCHF  --> TCA0
    OSCHF  --> USART0

    PIT      --> DISPATCH
    DISPATCH --> TASKS
    TASKS    --> FSM

    FSM --> ADC0
    FSM --> TCA0
    FSM --> AC0

    ADC0     --> RESRDY --> FSM
    AC0      --> AC_ISR --> FSM
    AC0      --> CH0_EV --> ADC0

    USART0   --> RXC_ISR --> FIFO_MOD
    FSM      --> USART0
    FIFO_MOD --> USART0

    VREF_ADC --> ADC0
    VREF_AC  --> AC0

    POT      --> ADC0
    POT      --> AC0
    TCA0     --> PWM_OUT
    TASKS    --> LED_HW
    USART0   --> USB
```
