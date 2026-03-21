# embedded_systems_project

PC Fan Controller Using STM32L432KC

Overview

This project implements a PC fan controller using the STM32L432KC microcontroller.

The controller provides:

1) Manual fan speed control via a potentiometer
2) PWM-based fan regulation for smooth speed adjustment
3) Real-time RPM monitoring using fan tachometer pulses with interrupts
4) Serial communication to display PWM duty cycle and fan speed on a terminal

The fan used is the FA07015L12LPB, a 12V DC fan with tachometer output.

Features: 

1) Manual fan speed adjustment using a potentiometer.
2) PWM-based fan control for precise speed regulation.
3) Fan RPM measurement using tachometer pulses, GPIO interrupts, and SysTick timer.
4) Serial output of PWM duty cycle and RPM for real-time monitoring.
5) Simple, low-cost hardware design without a display.

Hardware Requirements:
Component	       Specification
Microcontroller	       STM32L432KC (32-bit ARM Cortex-M4)
Fan	               FA07015L12LPB (12V DC, 3-pin with tachometer)
Potentiometer     	10kΩ (linear)
PWM Driver	       N-channel MOSFET or transistor for 12V fan
Power Supply	      12V for fan, 3.3V for MCU
Miscellaneous	     Jumper wires, breadboard

Software Requirements:
IDE:               STM32CubeIDE
STM32CubeMX:      Optional for pin and peripheral configuration
Programming Language:  C (Visual Code software)
Serial Terminal:      PuTTY, Tera Term, Arduino Serial Monitor, or similar
Libraries: STM32 HAL

Note:

The fan is powered via a MOSFET.
Tachometer output is connected to a GPIO configured for external interrupts.
PWM duty cycle controls fan speed; tachometer pulses measure RPM.

Firmware Architecture:

/src
    main.c      # Main loop: read potentiometer, update PWM, transmit serial data
    pwm.c       # PWM configuration and control
    adc.c       # ADC read for potentiometer
    serial.c    # UART serial output
    tach.c      # External interrupt handler for tachometer pulses
    tick.c      # SysTick handler for RPM calculation
/inc
    pwm.h
    adc.h
    serial.h
    tach.h
    tick.h

How It Works:

1) PWM Control: 
ADC reads potentiometer value.
Value is mapped to PWM duty cycle (0–100%).
PWM signal drives MOSFET, controlling fan speed.

2) RPM Measurement:

Fan tachometer output generates pulses per revolution (2 pulses/rev for FA07015L12LPB).
GPIO interrupt increments a pulse counter on each edge.
SysTick timer triggers periodically to calculate RPM:

RPM = (pulse_count / pulses_per_rev) * 60 / tick_period_seconds

3) Serial Communication:
PWM duty cycle and RPM are transmitted via UART.
Output is visible on any serial terminal program.
Installation & Setup
Connect STM32L432KC to PC via USB.
Connect potentiometer to ADC pin (PA0).
Connect fan via MOSFET; connect tachometer output to GPIO with interrupt (PB0).
Configure ADC, PWM, UART, and EXTI in STM32CubeIDE.
Compile and upload firmware.
Open a serial terminal at the configured baud rate (e.g., 115200).
Rotate potentiometer to adjust fan speed; monitor PWM duty cycle and RPM in real-time.

Troubleshooting
Fan does not spin: Verify MOSFET wiring, 12V supply, and PWM signal.
RPM not updating: Check tachometer connection and GPIO interrupt configuration.
No serial output: Confirm UART configuration and correct COM port in terminal.
