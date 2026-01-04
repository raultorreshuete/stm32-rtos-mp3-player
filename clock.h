/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión del reloj del sistema
  */

#ifndef CLOCK_H
#define CLOCK_H

	#include "stm32f4xx_hal.h"
	#include <stdint.h> // Para asegurar tipos uint8_t

		int Init_clock (void);          //Función de inicialización del hilo del reloj
    void clock(void *argument);     //Función del hilo del reloj

		//Estructura HoraCompleta
		typedef struct{ 
			uint8_t horas;
			uint8_t minutos;
			uint8_t segundos;
		} HoraCompleta;
		
		// --- CAMBIO IMPORTANTE: Exportamos la variable global ---
		extern HoraCompleta hora;

#endif // CLOCK_H