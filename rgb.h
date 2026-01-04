/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión de los leds RGB
  */
	
#ifndef _THLEDSRGB_
    
    #include "stm32f4xx_hal.h"
    
    #define _THLEDSRGB_
    #define MSGQUEUE_OBJECTS 4
    
    int Init_ThledsRGB(void);            //Función de inicialización del hilo
    void ThledsRGB (void *argument);     //Función del hilo 
      
    void initLEDsRGB(void);              //Función de inicialización de los LEDs

    typedef enum{
      RGB_OFF,         // Apagado 
      RGB_PLAY,        // verde parpadeando 4Hz
      RGB_PAUSE,       // azul parpadeando  1Hz
      RGB_ERROR        // Rojo parpadeando  5Hz
    } estadosRGB;   

    typedef struct{
      estadosRGB estado;
      uint8_t volumen;
    } infoRGB;

#endif
