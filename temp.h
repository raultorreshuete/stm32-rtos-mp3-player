/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión de la temperatura
  */


#ifndef TEMP_H
#define TEMP_H

#include "cmsis_os2.h"

#define LM75B_I2C_ADDR  0x48        // Direción del esclavo (datasheet)
#define MSGQUEUE_OBJECTS 4          //Define el tamaño de la cola

#define I2C_FLAG_DONE 0x01

//Estructura que propociona informacion sobre la temperatura que se va a leer
typedef struct{
  double temperatura;
}infoTemp;


extern osMessageQueueId_t Idcolatemp;
int Init_Temp(void);                // Función para la inicialización del hilo
void Thread_temp (void *argument);  // Función del hilo

void Init_I2C(void);                // Función para la inicialización del I2C

void myI2C_Callback(uint32_t event);



#endif /* TEMP_H*/




