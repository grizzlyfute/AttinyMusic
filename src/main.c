#ifndef F_CPU
//use fuse defined in Makefile
#define F_CPU 8000000UL
#endif

/************* INCLUDES ************/
#include "melody.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/cpufunc.h>

/************* iGLOBALES ************/
// volatile shall be used when variable is modified by interrupt and access
// somewhere else (or in the other way). Remove it provide more optimisation
static /*volatile*/ uint16_t counter = 0;
static /*volatile*/ uint16_t freq = 0;
static /*volatile*/ uint16_t timer = 0;
static uint8_t sub_timer = 0;

// table de periode 256 entre 0 et 255. En ram pour aller plus vite
const uint8_t sin_pcm[256] =
{
	128, 131, 134, 137, 140, 143, 146, 149,
	152, 155, 158, 162, 165, 167, 170, 173,
	176, 179, 182, 185, 188, 190, 193, 196,
	198, 201, 203, 206, 208, 211, 213, 215,
	218, 220, 222, 224, 226, 228, 230, 232,
	234, 235, 237, 238, 240, 241, 243, 244,
	245, 246, 248, 249, 250, 250, 251, 252,
	253, 253, 254, 254, 254, 255, 255, 255,
	255, 255, 255, 255, 254, 254, 254, 253,
	253, 252, 251, 250, 250, 249, 248, 246,
	245, 244, 243, 241, 240, 238, 237, 235,
	234, 232, 230, 228, 226, 224, 222, 220,
	218, 215, 213, 211, 208, 206, 203, 201,
	198, 196, 193, 190, 188, 185, 182, 179,
	176, 173, 170, 167, 165, 162, 158, 155,
	152, 149, 146, 143, 140, 137, 134, 131,
	128, 124, 121, 118, 115, 112, 109, 106,
	103, 100, 97, 93, 90, 88, 85, 82,
	79, 76, 73, 70, 67, 65, 62, 59,
	57, 54, 52, 49, 47, 44, 42, 40,
	37, 35, 33, 31, 29, 27, 25, 23,
	21, 20, 18, 17, 15, 14, 12, 11,
	10, 9, 7, 6, 5, 5, 4, 3,
	2, 2, 1, 1, 1, 0, 0, 0,
	0, 0, 0, 0, 1, 1, 1, 2,
	2, 3, 4, 5, 5, 6, 7, 9,
	10, 11, 12, 14, 15, 17, 18, 20,
	21, 23, 25, 27, 29, 31, 33, 35,
	37, 40, 42, 44, 47, 49, 52, 54,
	57, 59, 62, 65, 67, 70, 73, 76,
	79, 82, 85, 88, 90, 93, 97, 100,
	103, 106, 109, 112, 115, 118, 121, 124,
};


/************* INTERRUPTS  ************/
ISR(TIM0_COMPA_vect)
{
//	OCR1A = sin_pcm[256*freq*counter/SAMPLE_FREQ)]; //SAMPLE_FREQ = 32768
	OCR1A = sin_pcm[(counter >> 7) & 0xFF];
	counter += freq;

	--sub_timer;
	if (!sub_timer)
	{
		if (timer > 0) --timer;
		else TIMSK &= ~(1 << OCIE0A); // stop interrupt
	}
}

// PCInt[0-5] and external interrupt as the same vector
ISR(PCINT0_vect)
{
	// Stop current playing note
	TIMSK &= ~(1 << OCIE0A);
}


/************* LIBRAIRIES ************/
uint8_t customRand(uint8_t max, uint8_t *seed)
{
	// m = 256 = 2^8
	// c = 1
	// a - 1 = 4*31 = 124
	//
	// Linear congruential generator
	// X(n+1) = (a*X(n) + c) % m
    // m and the offset c are relatively prime,
    // a - 1 is divisible by all prime factors of m,
    // a - 1 is divisible by 4 if m is divisible by 4.

	*seed = (125*(*seed) + 1) & 0xFF;
	//return (uint8_t)((seed*MAX_MELODIES)/256);
	return (uint8_t)(((*seed)*max) >> 8);
}

// force PWM value to be 0 (avoid discharging battery)
void resetPWMPin(void)
{
	TCCR1 |= (1 << CS11);

	OCR1A = 255;
	TCNT1 = 255;
	_NOP();
	_NOP();
	_NOP();
	// stop timer 1
	TCCR1 &= ~((1 << CS10) | (1 << CS11) | (1 << CS12) | (1 << CS13));
	PORTB &= (0 << PIN1); //force value to 0 (avoid discharging battery when not use)
}

uint16_t watchForButton(void)
{
	uint16_t r = 0;
	while (PINB & (1 << PIN4))
	{
		r++;
		// anti rebonds
		_delay_us(1);
	}
	return r;
}


/************* ROUNTIMEs ************/

void setNote(uint16_t frequency, uint8_t duration10ms)
{
	if (duration10ms <= 0) return;

	TIMSK &= ~(1 << OCIE0A);

	// choose interrupt frequency (sampling frequency) at 32768 Hz. (min output
	// frequency is 122 Hz)
	// (sysclok = 8MHz)
	// f_sin = frequency / f_min, f_min = f_interupt / 256
	OCR0A = 244;

	const uint16_t prescalars[6] = {0, 1, 8, 64, 256, 1024};
	const uint8_t prescalar = 1;
	freq = frequency;

	// Interruption frequency is F_CPU/(tmp*ocra)
	//timer = (uint32_t)(duration10ms*F_CPU)/(tmp*OCR0A*100);
	//==> fail with tmp = 1024, duration10ms = 1, F_CPU = 8e6, OCR0A = 8
	uint32_t tmp = (uint32_t)(duration10ms)*F_CPU;
	tmp /= (uint32_t)(OCR0A);
	tmp /= (uint32_t)(prescalars[prescalar]);
	tmp /= 100UL;
	// timer is decremented every 256 cycles
	sub_timer = tmp & 0xFF;
	timer = (uint16_t)(tmp >> 8);

	TCCR0B = (TCCR0B &
			~((1 << CS02) | (1 << CS01) | (1 << CS00))) |
		prescalar;

	// start pwm
	if (frequency > 0)
		//use 2 as presacalar (PWM at 32 MHz / 255)
		TCCR1 |= (1 << CS11);

	// start timer0 interrupt - enable interrupt on compare A
	TIMSK |= (1 << OCIE0A);
	do
	{
	} while (TIMSK & (1 << OCIE0A));

	//stop timer 0
	TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));

	// stop timer 1
	TCCR1 &= ~((1 << CS10) | (1 << CS11) | (1 << CS12) | (1 << CS13));
}

int main(void)
{
	cli();
	/* PWM configuration */
	// PWM out at PB1 (timer 1). Should be defined as output and as 'HIGH' as default
	DDRB |= (1 << PIN1);

	// Use timer 0 for sampling
	// Set CTC Mode (clear on compare OCR0A)
	TCCR0A |= (1 << WGM01);

	/* TIMER 1 for changing sampling */
	// prescalar will be computed when changing note

	// clear on compare match OCR1C
	TCCR1 |= (1 << CTC1);
	// enable PWM (set ON OCR1A, reset on OCR1C)
	TCCR1 |= (1 << PWM1A);
	// set on compare match. Reset at bottom
	TCCR1 |= (1 << COM1A1) | (1 << COM1A0);
	// set TOP at 0xFF
	OCR1C = 0xFF;
	// PLL is a "clock multiplier" (switch off in sleep mode)
	PLLCSR |= (1 << PLLE);
	// wait for PLOCK
	while (~PLLCSR & (1<<PLOCK)) {};
	// use pck (clock asyncronous as 64 MHz)
	PLLCSR |= (1 << PCKE);

	/* BUTTON CONFIGURATION */
	DDRB &= ~(1 << PIN4);
	// activate internal pull up
	PORTB |= (1 << PIN4);
	MCUCR &= ~(1 << PUD);
	// Enable interrupt on PIN4 for waking up
	GIMSK |= (1 << PCIE);
	PCMSK |= (1 << PCINT4);

	/* SLEEP CONFIGURATION */
	// disable Burn out detection
	MCUCR |= (1 << BODS);
	// wakeup only if usi start condition or pin change
	MCUCR |= (1 << SM1);
	MCUCR &= ~(1 << SM0);
	// USI power reduction
	PRR |= (1 << PRUSI);
	// ADC power reduction
	PRR |= (1 << PRADC);

	sei();

	resetPWMPin();

	/* PROGRAM BEGIN */
	uint8_t i;
	uint16_t j;
	uint8_t seed = 0;
	Note note;

	do
	{
		// enter to sleep
		// 1. Disable timer0 & timer1
		// 2. Enable sleep
		// 3. sleep
		// 4. Disable sleep
		// 5. Enable timer0 & timer1
		// 6. reconfigure PLL
		PRR |= (1 << PRTIM1) | (1 << PRTIM0);
		sleep_enable();
		sleep_cpu(); //pin change interrupt wake up the mcu
		sleep_disable();
		PRR &= ~((1 << PRTIM1) | (1 << PRTIM0));
		// reconfigure clock multiplier, deconfigured during sleep
		PLLCSR |= (1 << PLLE);
		while (~PLLCSR & (1<<PLOCK)) {};
		PLLCSR |= (1 << PCKE);

		//wait for released for setting seed
		j = watchForButton();
		if (j > 1000)
		{
			if (seed == 0) seed = (j & 0xFF);

			i = customRand (MAX_MELODIES, &seed);
			j = 0;
			do
			{
				memcpy_PF(&note, (pgm_read_word(&melodies[i]) + j*sizeof(Note)), sizeof(Note));
				j++;
				setNote(pgm_read_word(&frequencyTable[note.frequency]), note.duration);
			} while (note.duration > 0 && watchForButton() < 1);
			resetPWMPin();
		}
	} while (1);
}
