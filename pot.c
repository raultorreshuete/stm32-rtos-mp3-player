/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
 - Módulo encargado de leer los valores del volumen del potenciómetro POT_2
 */

#include "cmsis_os2.h"  // CMSIS RTOS header file
#include "pot.h"        // Header del módulo
#include "stdio.h"      // Para printf o debugging (opcional)
#include "stm32f4xx_hal.h"
#include "math.h"

/*----------------------------------------------------------------------------
 * Thread 'Thread_pot': Hilo que gestiona la lectura del POT_2
 *---------------------------------------------------------------------------*/

// Identificadores de RTOS
 osThreadId_t Idpot;
 osMessageQueueId_t IdColaPot;

// Estructura Handler del ADC
ADC_HandleTypeDef AdcHandle; 

// Variables de estado
static uint32_t ultimo_valor = 0; // Almacena la última lectura para detectar cambios

// 1. Funciones de Configuración del ADC (Basadas en las diapositivas)
/**
 Configura el pin PC0 como entrada analógica (ADC1_IN10).
 */
void POT_ADC_Pins_Config(void) {
    GPIO_InitTypeDef GPIO_InitStruct;

    // Habilitar reloj del ADC y del GPIO
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE(); 

    // Configuración del pin PC0
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/**
 *  Inicializa el periférico ADC1 para conversión simple por polling.
 *  hadc Puntero al handler del ADC.
 *  ADC_Instance Instancia del ADC a usar (ADC1).
 * 
 */
int POT_ADC_Init_Single_Conversion(ADC_HandleTypeDef *hadc, ADC_TypeDef *ADC_Instance) {
    hadc->Instance = ADC_Instance;
    hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc->Init.Resolution = ADC_RESOLUTION_12B;
    hadc->Init.ScanConvMode = DISABLE;
    hadc->Init.ContinuousConvMode = DISABLE;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc->Init.NbrOfConversion = 1;
    hadc->Init.DMAContinuousRequests = DISABLE;
    hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    if (HAL_ADC_Init(hadc) != HAL_OK) {
        return -1;
    }
    return 0;
}

/**
 * @brief Realiza la lectura del ADC en el canal especificado.
 * @param hadc Puntero al handler del ADC.
 * @param Channel Canal ADC a leer (ADC_CHANNEL_10).
 * @return El valor ADC crudo (uint32_t).
 */
static uint32_t POT_ADC_Read_Raw(ADC_HandleTypeDef *hadc, uint32_t Channel) {
    ADC_ChannelConfTypeDef sConfig;
    HAL_StatusTypeDef status;
    uint32_t raw = 0;

    sConfig.Channel = Channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES; // Tiempo de muestreo rápido

    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
        return 0xFFFFFFFF; // Valor de error
    }

    HAL_ADC_Start(hadc);

    // Esperamos a la conversión por polling (sin timeout)
    status = HAL_ADC_PollForConversion(hadc, 0); 
    
    // Solo si la conversión fue exitosa, obtenemos el valor
    if (status == HAL_OK) {
        raw = HAL_ADC_GetValue(hadc);
    }
    
    return raw;
}

// Función de Inicialización del Hilo
int Init_POT (void) {

    POT_ADC_Pins_Config();  //Llamada a la inicialización física de los pines y del potenciómetro
    if (POT_ADC_Init_Single_Conversion(&AdcHandle, ADC1) != 0) {
         return -1; 
    }
    
    IdColaPot = osMessageQueueNew(MSGQUEUE_OBJECTS_POT, sizeof(infoPot), NULL);
    if (IdColaPot == NULL){
      return -1;
    }

    Idpot = osThreadNew(POT_2, NULL, NULL);
    if (Idpot == NULL) {
        return(-1); 
    }
    
    return(0);
}

//Función del hilo del potenciómetro
void POT_2 (void *argument) {
    infoPot datosPot;
    uint32_t valor;
    
    osDelay(100); 

    while (1) {
        
        // Leemos el valor ADC
        valor = POT_ADC_Read_Raw(&AdcHandle, ADC_CHANNEL_POT);
        
        // Sólo operamos si se detecta un cambio significativo (Para evitar enviar spam de mensajes por ruido)
        // (Se considera un cambio significativo si el valor ha variado más de 10 unidades (ajustable))
        if (valor != 0xFFFFFFFF && (valor > ultimo_valor + 100 || valor < ultimo_valor - 100)) {
            
            // Convertimos a nivel de volumen (Normalización)
            // Normalizamos el valor (0-4095) al nivel de volumen (0 a 30)
            datosPot.value = valor;
            datosPot.nivel_volumen = (uint8_t)roundf((float)valor * 30.0f / ADC_MAX_VALUE_12BIT);
            
            // Nos aseguramos de no sobrepasar el límite de volumen del fabricante (30)
            if (datosPot.nivel_volumen > 30) {
                datosPot.nivel_volumen = 30;
            }

            // Envíamos los datos importantes a la cola
            osMessageQueuePut(IdColaPot, &datosPot, 0U, 0U);

            // Actualizamos el último valor
            ultimo_valor = valor;
        }

        // Muestreamos cada 100ms
        osDelay(250);
    }
}