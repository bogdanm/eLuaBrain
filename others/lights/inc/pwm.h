// PWM functions

#ifndef __PWM_H__
#define __PWM_H__

#include "machine.h"

void pwm_init();
void pwm_start();
void pwm_stop();
void pwm_set_frequency( u32 freq );
void pwm_set_duty( int duty );

#endif
