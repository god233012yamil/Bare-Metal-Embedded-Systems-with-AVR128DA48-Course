# Project Flowchart

This document describes the runtime flow for the AVR128DA48 callback and
event-driven demonstration.

## System Initialization

```mermaid
flowchart TD
    Start([Reset / main enters]) --> AppInit["app_init()"]
    AppInit --> DisableInterrupts["cli(): disable global interrupts"]
    DisableInterrupts --> BoardInit["board_init()\nConfigure PC6 LED and PC7 button"]
    BoardInit --> EventInit["app_event_init()\nReset event queue indexes and drop counter"]
    EventInit --> ButtonInit["button_init()\nClear registered callback and debounce state"]
    ButtonInit --> TimerInit["software_timer_init()\nStop timer and clear callback state"]
    TimerInit --> TickInit["system_tick_init()\nConfigure TCB0 for 1 ms interrupts"]
    TickInit --> RegisterCallback["button_register_callback(app_button_callback, NULL)"]
    RegisterCallback --> SleepMode["set_sleep_mode(SLEEP_MODE_IDLE)"]
    SleepMode --> EnableInterrupts["sei(): enable global interrupts"]
    EnableInterrupts --> MainLoop["Enter main event loop"]
```

## Main Event Loop

```mermaid
flowchart TD
    LoopStart["for (;;)"] --> PopEvent{"app_event_pop(&event)"}
    PopEvent -->|Event available| ProcessEvent["app_process_event(&event)"]
    PopEvent -->|Queue empty| Sleep["sleep_mode()\nCPU idles until interrupt"]
    Sleep --> LoopStart

    ProcessEvent --> EventType{"event.type"}
    EventType -->|APP_EVENT_BUTTON_PRESSED| ToggleLed["board_led_toggle()"]
    ToggleLed --> StartTimer["software_timer_start(1000 ms,\napp_timeout_callback, NULL)"]
    StartTimer --> LoopStart

    EventType -->|APP_EVENT_TIMEOUT| LedOff["board_led_off()"]
    LedOff --> LoopStart

    EventType -->|APP_EVENT_NONE / default| Ignore["No action"]
    Ignore --> LoopStart
```

## Button Press Event Path

```mermaid
flowchart TD
    ButtonPress([User presses active-low PC7 button]) --> PortInterrupt["PORTC_PORT_vect ISR"]
    PortInterrupt --> CaptureFlags["Read PORTC.INTFLAGS"]
    CaptureFlags --> ClearFlags["Clear captured interrupt flags"]
    ClearFlags --> Pin7Flag{"PIN7 flag set?"}
    Pin7Flag -->|No| ExitPortIsr([Return from ISR])
    Pin7Flag -->|Yes| ButtonIrq["button_irq_handler()"]

    ButtonIrq --> ReadTime["system_tick_get_from_isr()"]
    ReadTime --> ReadButton{"board_button_is_pressed()?"}
    ReadButton -->|No| ExitPortIsr
    ReadButton -->|Yes| Debounce{"Accepted before and\nelapsed < 50 ms?"}
    Debounce -->|Yes| ExitPortIsr
    Debounce -->|No| SaveTime["Update last_accepted_press_ms"]
    SaveTime --> HasCallback{"registered_callback != NULL?"}
    HasCallback -->|No| ExitPortIsr
    HasCallback -->|Yes| InvokeCallback["app_button_callback(BUTTON_ID_USER, context)"]

    InvokeCallback --> BuildEvent["Create APP_EVENT_BUTTON_PRESSED event"]
    BuildEvent --> PushEvent["app_event_push_from_isr(event)"]
    PushEvent --> QueueFull{"Event queue full?"}
    QueueFull -->|Yes| DropEvent["Increment dropped_event_count"]
    QueueFull -->|No| StoreEvent["Store event and advance head index"]
    DropEvent --> ExitPortIsr
    StoreEvent --> ExitPortIsr
```

## Timer Timeout Event Path

```mermaid
flowchart TD
    Tick([TCB0 reaches 1 ms compare]) --> TickIsr["TCB0_INT_vect ISR"]
    TickIsr --> ClearTcbFlag["Clear TCB0 interrupt flag"]
    ClearTcbFlag --> TickHandler["system_tick_irq_handler()"]
    TickHandler --> IncrementMs["Increment system_milliseconds"]
    IncrementMs --> TimerTick["software_timer_tick_1ms()"]

    TimerTick --> TimerRunning{"timer_running?"}
    TimerRunning -->|No| ExitTickIsr([Return from ISR])
    TimerRunning -->|Yes| RemainingPositive{"remaining_ms > 0?"}
    RemainingPositive -->|Yes| Decrement["remaining_ms--"]
    RemainingPositive -->|No| CheckExpired{"remaining_ms == 0?"}
    Decrement --> CheckExpired

    CheckExpired -->|No| ExitTickIsr
    CheckExpired -->|Yes| StopTimer["timer_running = false"]
    StopTimer --> LoadCallback["Copy timer_callback and timer_context"]
    LoadCallback --> CallbackValid{"callback != NULL?"}
    CallbackValid -->|No| ExitTickIsr
    CallbackValid -->|Yes| TimeoutCallback["app_timeout_callback(context)"]

    TimeoutCallback --> BuildTimeout["Create APP_EVENT_TIMEOUT event"]
    BuildTimeout --> PushTimeout["app_event_push_from_isr(event)"]
    PushTimeout --> TimeoutQueueFull{"Event queue full?"}
    TimeoutQueueFull -->|Yes| DropTimeout["Increment dropped_event_count"]
    TimeoutQueueFull -->|No| StoreTimeout["Store event and advance head index"]
    DropTimeout --> ExitTickIsr
    StoreTimeout --> ExitTickIsr
```

## Module Responsibilities

```mermaid
flowchart LR
    Board["board.c\nGPIO setup, LED control,\nbutton read"] --> Button["button.c\nDebounce and callback dispatch"]
    Button --> Main["main.c\nRegisters callbacks and\nprocesses deferred events"]
    Tick["system_tick.c\n1 ms hardware tick"] --> Timer["software_timer.c\nOne-shot timeout callback"]
    Timer --> Main
    Main <--> Events["app_event.c\nInterrupt-safe ring buffer"]
    Interrupts["interrupts.c\nPORTC and TCB0 vectors"] --> Button
    Interrupts --> Tick
```
