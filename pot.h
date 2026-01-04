/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
 - Módulo encargado de leer los valores del volumen del potenciómetro POT_2
 */

#ifndef POT_H
#define POT_H

#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"


#define MSGQUEUE_OBJECTS_POT 4                // Tamaño de la cola para los mensajes
#define ADC_CHANNEL_POT      ADC_CHANNEL_10   // PC0 -> ADC1_IN10


// Rango de salida del ADC (12 bits) y Voltaje de Referencia
#define ADC_MAX_VALUE_12BIT   4096.0f   
#define VREF_VOLTAGE         3.3f       // Voltaje de referencia de 3.3V

// Estructura que propociona información del volumen
typedef struct{
    uint32_t value;         // Valor digital (entre 0 y 4095)
    uint8_t nivel_volumen;  // Nivel de volumen normalizado (entre 0 y 30)
} infoPot;

int Init_POT(void);                 // Función para la inicialización del hilo
void POT_2 (void *argument);        // Función del hilo

// Funciones de inicialización del ADC (obtenidas de las diapositivas)
void POT_ADC_Pins_Config(void);
int POT_ADC_Init_Single_Conversion(ADC_HandleTypeDef *hadc, ADC_TypeDef *ADC_Instance);
float POT_ADC_getVoltage(ADC_HandleTypeDef *hadc, uint32_t Channel);



#endif /* POT_H*/