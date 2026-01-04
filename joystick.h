/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión del joystick y sus rebotes
  */
	
#ifndef _JOYSTICK_

	#include "stm32f4xx_hal.h"
	
	#define _JOYSTICK_
  
  #define JOYS_PULSADO 0x01U          //Flag que indica que el joystick se ha pulsado
	#define TIMER_REBO 0x02U          //Flag que indica que el timer de los rebotes ha terminado 
	
		int Init_joystick(void);            //Función de inicialización del hilo
    void joystick (void *argument);     //Función del hilo 
    
    void EXTI15_10_IRQHandler(void);  //Función de la interrupción para el joystick
		void initJoys(void);              //Función de inicialización del joystick
    
    
    //Prototipo del Callback del timer de 1 seg que gestiona el tipo de pulsación
    void Callback_timer1segR (void const *arg);
    void Callback_timer1segL (void const *arg);
    void Callback_timer1segC (void const *arg);


    //Estructura InfoJoys, se introducirá en la cola que va al principal.
    // Proprociona información sobre que acción se ha pulsado en el josytick,
    // y si esta es larga (tipo_pulsacion = 1) o corta (tipo_pulsacion = 0)
    typedef struct{
      uint16_t GPIO_PIN;
      uint8_t tipo_pulsacion;
    } infoJoys;
		
		
		//Declaraciones del hilo de los rebotes
		int Init_ThtimerRebo(void);            //Función de inicialización del hilo
    void ThtimerRebo (void *argument);     //Función del hilo 
    
    void Callback_timerRebo (void const *arg);   //Callback del timer virtual de los rebotes
    
		
#endif
