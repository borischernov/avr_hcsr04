# avr_hcsr04 - Digital output for HC-SR04 Ultrasonic Ranging Module

Code for ATtiny45 interfacing [HC-SR04](http://www.micropik.com/PDF/HCSR04.pdf) module and sending measurement results via UART.
Default UART speed is 9600 baud. Measurement results (in cm) are sent one-per-line. 

Use avr-gcc to compile the code, or just program included HEX file into ATtiny45.

Fuse bytes:
low: 0xEE
high: 0xDF
ext: 0xFF