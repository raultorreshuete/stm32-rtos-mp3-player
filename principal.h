/**
SBM - PROYECTO
Raúl Torres Huete
Roberto Vila García
(M01M02J05J06B)
  */

#ifndef PRINCIPAL_H

	#include "stm32f4xx_hal.h"
  
  #define FLAG_SEGUNDO_NUEVO 0x00000010U
  
	#define PRINCIPAL_H

		int Init_principal (void);

		// Función para actualizar el reloj cada segundo
		void Thprincipal(void *argument);
		


#endif // PRINCIPAL_H
