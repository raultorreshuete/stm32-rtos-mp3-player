/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión del PWM
  */

#ifndef PWM_H
	
#include "stm32f4xx_hal.h"

	#define PWM_H

    int Init_PWM (void);
    void PWM(void *argument);
    
    void initpwm(void);
		
		void initPINOUT(void);
		void initTIM1(void);
		
		int Init_Test_PWM(void);
		void TestPWM(void *argument);

#endif
