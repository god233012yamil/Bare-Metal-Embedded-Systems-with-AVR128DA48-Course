#ifndef CONFIG_H_
#define CONFIG_H_

#ifndef F_CPU
#define F_CPU 24000000UL
#endif

#define UART_BAUD_RATE                 115200UL

#define LED_HEARTBEAT_PORT             VPORTB
#define LED_HEARTBEAT_PIN              PIN3_bm

#define FAULT_BUTTON_PORT              VPORTC
#define FAULT_BUTTON_PIN               PIN7_bm
#define FAULT_BUTTON_PINCTRL           PORTC.PIN7CTRL

#define SENSOR_PERIOD_MS               100UL
#define CONTROL_PERIOD_MS              50UL
#define COMM_PERIOD_MS                 250UL
#define DIAGNOSTICS_PERIOD_MS          1000UL
#define HEARTBEAT_PERIOD_MS            500UL

#define UART_TX_TIMEOUT_MS             10UL
#define SYSTEM_RECOVERY_DELAY_MS       250UL

#define MAX_FAILED_BOOTS               3U
#define STACK_LOW_WATERMARK_BYTES      128U

#endif
