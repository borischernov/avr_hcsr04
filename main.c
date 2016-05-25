/*
 * main.c
 *
 *  Created on: 24/5/2016
 *      Author: boris
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

#define F_CPU			8000000UL

#define UART_TXD_PIN 	PB0
#define UART_BAUD		9600
#define UART_BITS		12		// Total number of bits, including start, data, stop, inter-character space

#define TRIG_PIN		PB1
#define ECHO_PIN		PB2		// Use PB2 (INT0)

#define TRIG_LENGTH		30		// Trigger pulse length (uS)
#define US_PER_CM		58		// 58uS / cm
#define MEASURE_TIME_MS	50		// Measurement interval

volatile static uint8_t uart_out;
volatile static uint8_t uart_bit = 0;
volatile static uint8_t uart_data;

volatile static uint16_t pulse_length = 0;
volatile static uint16_t result;

// UART Timer Interrupt
ISR(SIG_OUTPUT_COMPARE0A) {
	// Output a bit
	if (uart_out & 1)
		PORTB |= _BV(UART_TXD_PIN);
	else
		PORTB &= ~_BV(UART_TXD_PIN);

	uart_bit--;

	if (!uart_bit) {				// Check if tx done
		TIMSK &= ~_BV(OCIE0A);  	// Disable Interrupt TimerCounter0 Compare Match A
		return;
	}

	if (uart_bit == UART_BITS - 1)
		uart_out = uart_data;		// Start bits transmitted, now goes the rest
	else {
		uart_out >>= 1;
		uart_out |= 0x80;			// Prepare 1s for stop bits
	}
}

// Measurement Timer Interrupt
ISR(SIG_OUTPUT_COMPARE1A) {
	pulse_length++;
}

// Echo Input Interrupt
ISR(SIG_INTERRUPT0) {
	if (PINB & _BV(ECHO_PIN)) {  // Pulse start
		TCNT1 = 0;
		pulse_length = 0;
	} else						 // Pulse end
		result = pulse_length;
}

void uart_setup(void) {
	// GPIO configuration
	DDRB |= _BV(UART_TXD_PIN);		// Configure TxD pin as output
	PORTB |= _BV(UART_TXD_PIN);		// UART space is 1

	// Timer 0 configuration
	TCCR0A = _BV(WGM01);			// CTC Mode
	TCCR0B = _BV(CS01);				// Clock = ClkI/O  / 8
	OCR0A = F_CPU / 8 / UART_BAUD;	// Timer reload value
}

void sensor_setup(void) {
	DDRB |= _BV(TRIG_PIN);			// Trig pin is output
	PORTB &= ~_BV(TRIG_PIN);		// Set Trig to 0
	DDRB &= ~_BV(ECHO_PIN);			// Sensor pin is input
	PORTB |= _BV(ECHO_PIN);			// Enable pull-up resistor

	// Timer 1 configuration
	TCCR1 = _BV(CTC1) | _BV(CS12);	// CTC Mode, Clock = ClkI/O / 8
	OCR1C = US_PER_CM - 1;
	TIMSK |= _BV(OCIE1A);  			// Enable Interrupt TimerCounter1 Compare Match A

	// Setup INT0
	MCUCR |= _BV(ISC00);			// Interrupt on any logical change on INT0
	GIMSK |= _BV(INT0);				// Enable external pin interrupt
}

void uart_send(uint8_t data) {
	if (uart_bit)
		return;
	uart_data = data;
	uart_bit = UART_BITS;
	uart_out = 0;					// Start bit is logic 0
	TIFR |= _BV(OCF0A);				// Clear OCF0A by writing logic 1
	TIMSK |= _BV(OCIE0A);  			// Enable Interrupt TimerCounter0 Compare Match A
}

void uart_puts(char *str) {
	char *ptr;

	for (ptr = str; *ptr; ptr++) {
		while (uart_bit);
		uart_send((uint8_t)*ptr);
	}
	while (uart_bit);
	uart_send('\r');
	while (uart_bit);
	uart_send('\n');
}

void measure_distance(void) {
	result = 0;

	// Trig pulse
	PORTB |= _BV(TRIG_PIN);
	_delay_us(TRIG_LENGTH);
	PORTB &= ~_BV(TRIG_PIN);

	// Wait for echo
	_delay_ms(MEASURE_TIME_MS);

	if (result > 400)
		result = 0;
}

int main(void){
	static char res[8];

	uart_setup();
	sensor_setup();

	sei();

	while(1) {
		measure_distance();
		utoa(result, res, 10);
		uart_puts(res);
	}
}
