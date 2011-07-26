#include "pwm.h"
#include "machine.h"
#include <avr/io.h>

//******************************************************************************
// Timer definitions
#define start_timer()                     TCCR1B  = ( TCCR1B & ( u8 )~0x07 ) | PWM_PRESCALER 
#define stop_timer()                      TCCR1B &= ~0x07

// *****************************************************************************
// PWM functions

// Initialize PWM
// Output will be on OC1A (PB1)
void pwm_init()
{
  stop_timer();
  // Mode = 1110 (fast PWM with TOP in ICR1)
  // OC1A: clear on compare match, set on BOTTOM (non-inverting mode)
  TCCR1B |= _BV( WGM13 ) | _BV( WGM12 );
  TCCR1A |= _BV( WGM11 ) | _BV( COM1A1 );
  // Set OCR1A (PB1) as output
  PORTB &= ~_BV( PB1 );
  DDRB |= _BV( PB1 );
}

// Start PWM
void pwm_start()
{
  start_timer(); 
}

// Stop PWM
void pwm_stop()
{
  stop_timer();
  TCNT1 = 0;
}

// Set PWM frequency
void pwm_set_frequency( u32 freq )
{
  ICR1 = ( PWM_CLOCK / freq ) - 1;
}

// Setup PWM duty cycle (from 0% to 100%)
void pwm_set_duty( int duty )
{ 
  OCR1A = ( ICR1 * duty ) / 100;  
}
