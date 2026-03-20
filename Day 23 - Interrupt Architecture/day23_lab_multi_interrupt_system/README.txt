=====================================================================
  Lab: Dual Interrupts - RTC (1s) + ADC Conversion Complete
  Target : AVR128DA48 Curiosity Nano
  IDE    : Atmel Studio 7 / Microchip Studio 7
  Pack   : AVR-Dx Device Pack 2.4.286
=====================================================================

HOW TO OPEN
-----------
1. Open Atmel Studio / Microchip Studio
2. File -> Open -> Project/Solution
3. Navigate to this folder and open: RTC_ADC_Lab.atsln
4. The project RTC_ADC_Lab will appear in the Solution Explorer

HOW TO BUILD
------------
1. Build -> Build Solution  (or press F7)
2. Confirm 0 errors in the Output window

HOW TO PROGRAM
--------------
1. Connect the AVR128DA48 Curiosity Nano via USB
2. Tools -> Device Programming  (or Shift+Alt+F5)
   - Tool : PKOB nano (the onboard debugger)
   - Device: AVR128DA48
   - Interface: UPDI
3. Click "Apply", then "Read" to verify connection
4. In the Memories section, load the .hex from:
     RTC_ADC_Lab\Debug\RTC_ADC_Lab.hex
5. Click "Program"

EXPECTED BEHAVIOUR
------------------
- LED0 (PC6) blinks: ON 1 second, OFF 1 second (0.5 Hz)
- ADC0 continuously samples AIN0/PD0 in the background
- Main loop stays idle; all work is done in ISRs

PIN REFERENCE (Curiosity Nano)
------------------------------
  LED0  = PC6  (active-LOW: LOW = on)
  SW0   = PC7  (active-LOW, not used in this lab)
  ADC   = PD0  (AIN0, floating input for demo)

INTERRUPTS
----------
  RTC_PIT_vect   - fires every 1.000 s  (OSCULP32K / CYC32768)
  ADC0_RESRDY_vect - fires ~3800x/s     (4MHz / 32 prescaler, 12-bit)
=====================================================================
