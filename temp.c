/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión de la temperatura
  */


#include "cmsis_os2.h"  // CMSIS RTOS header file
#include "temp.h"
#include <math.h>
#include "stm32f4xx.h"
#include "Driver_I2C.h"
#include "stdio.h"

/*----------------------------------------------------------------------------
 *      Thread  'Thread_Temp': Hilo que gestiona la temperatura
 *---------------------------------------------------------------------------*/
 

osThreadId_t Idtemp;                 // thread id
osMessageQueueId_t Idcolatemp;              //Id de la cola
infoTemp datostemp;                   //En datosTemp se almacenará la temperatura

//Declaracion del driver I2C
extern ARM_DRIVER_I2C Driver_I2C1;
static ARM_DRIVER_I2C *I2Cdev = &Driver_I2C1;

// Inicializacion del I2C
void Init_I2C(void){
  I2Cdev->Initialize(myI2C_Callback);
  I2Cdev->PowerControl(ARM_POWER_FULL); 
  I2Cdev->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
  I2Cdev->Control(ARM_I2C_BUS_CLEAR, 0);
}

// Función de inicialización del hilo 
int Init_Temp (void) {
  Init_I2C(); // Inicializamos la comunicación I2C
  Idcolatemp = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(infoTemp), NULL);
  
 
  Idtemp = osThreadNew(Thread_temp, NULL, NULL);
  if (Idtemp == NULL) {
    return(-1);
  }
 
  return(0);
}
 
// Función del hilo temperatura
void Thread_temp (void *argument) {
  
  uint8_t read_temp[2]; // Buffer de lectura para el registro de la temperatura
  uint8_t rg_temp =0x00;
  double temp,tempc;  
  
  
  
 
  while (1) {
    osDelay(1000);
    
    I2Cdev->MasterTransmit(LM75B_I2C_ADDR, &rg_temp, 1, true);
    osThreadFlagsWait(I2C_FLAG_DONE, osFlagsWaitAny, osWaitForever);
    // Leemos el registro de la temperatura del sensor LM75B de la tarjeta mediante la comunicación I2C
    I2Cdev->MasterReceive(LM75B_I2C_ADDR, read_temp, 2, false);
    //Espera hasta que la transaccion de la comunicacion I2C termine
    osThreadFlagsWait(I2C_FLAG_DONE, osFlagsWaitAny, osWaitForever);
    
    //Convertimos los valores leidos de la tarjeta a valores de temperatura 
    temp = ((read_temp[0] << 8) | read_temp[1]) >> 5; // Temperatura en 11 bits
    tempc = temp*0.125; // Convertimos el valor segun el datasheet
    datostemp.temperatura = round(tempc*2)/2; // redondeamos 
    
    //Metemos el valor de la temperatura en la cola de mensajes
    osMessageQueuePut(Idcolatemp,&datostemp,NULL,0); 
    
  }
}


// --- CALLBACK DEL DRIVER I2C ---
// Simplificado: Cualquier evento de finalización (éxito o error) despierta al hilo.
void myI2C_Callback(uint32_t event) {
    // Levantamos el flag tanto si termina bien (TRANSFER_DONE) 
    // como si hay error (TRANSFER_INCOMPLETE, etc.) para que el hilo no se bloquee eternamente.
    if (event & (ARM_I2C_EVENT_TRANSFER_DONE | ARM_I2C_EVENT_TRANSFER_INCOMPLETE | ARM_I2C_EVENT_BUS_ERROR)) {
        osThreadFlagsSet(Idtemp, I2C_FLAG_DONE);
    }
}


