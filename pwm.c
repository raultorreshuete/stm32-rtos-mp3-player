/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión del PWM (Buzzer)
  */

#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "pwm.h"

osThreadId_t tid_PWM;
osMessageQueueId_t IdcolaPWM;       //Declaración del id de la cola

uint8_t tipo_pitido;      //Indica si pitido de play/pause -> 0
                          //       si pitido de no SD      -> 1
                                                    
uint32_t duracion_bip = 150;
uint32_t duracion_silencio = 300;

osThreadId_t tid_TestPWM;           // ID del hilo de test

#define MSGQUEUE_OBJECTS 4 // Aumentado por seguridad

TIM_OC_InitTypeDef TIM_Channel_InitStruct;


// --- CAMBIO IMPORTANTE: Usamos TIM1 en lugar de TIM2 como en la P2 ---
// El pin PE9 solo soporta TIM1_CH1 (AF1), no soporta TIM2.
static TIM_HandleTypeDef htim1; 

// Prototipos locales actualizados
void initPINOUT(void);

//Función inicialización del hilo del PWM
int Init_PWM (void) {
        
    initpwm();
  
    IdcolaPWM = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(uint8_t), NULL); 
    
    tid_PWM = osThreadNew(PWM, NULL, NULL);
    if (tid_PWM == NULL) {
        return(-1);
    }
 
    return(0);
}

//Función del hilo del PWM
void PWM (void *argument) {
  
    while (1) {

        osMessageQueueGet(IdcolaPWM, &tipo_pitido, NULL, osWaitForever);
      
        if(tipo_pitido == 0){
            // ---------------------------------------------------------
            // CASO 0: Play/Pause -> UN SOLO BIP AGUDO
            // ---------------------------------------------------------

            //Re-Configuramos el Timer (TIM1)
            //Nota: Al cambiar a TIM1 (APB2), la frecuencia base es el doble.
            //Mantenemos los periodos pero el prescaler se ha duplicado en la init.
            htim1.Init.Period = 102;
            HAL_TIM_PWM_Init(&htim1);
            TIM_Channel_InitStruct.Pulse = 52;                         
            HAL_TIM_PWM_ConfigChannel(&htim1, &TIM_Channel_InitStruct, TIM_CHANNEL_1);


            //Ejecutamos el sonido
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
            osDelay(duracion_bip); 
            HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
            
        } else {
            // ---------------------------------------------------------
            // CASO 1: No se detecta SD -> BIP PERIÓDICO GRAVE
            // ---------------------------------------------------------

            //Re-Configuramos el Timer (TIM1) con los valores para la nueva frecuencia
            htim1.Init.Period = 20;
            HAL_TIM_PWM_Init(&htim1);
            TIM_Channel_InitStruct.Pulse = 10;                         
            HAL_TIM_PWM_ConfigChannel(&htim1, &TIM_Channel_InitStruct, TIM_CHANNEL_1);

            //Ejecutamos el bucle de alarma
            // (básicamente vamos a reproducir un pitido más graves 3 veces seguidas para notificar)
            for(int i = 0; i < 3; i++) {
                 HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
                 osDelay(duracion_bip + 100); 
                 HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
                 
                 osDelay(duracion_silencio); 
            }
        }
        
        osThreadYield();
    }
}

//Función que engloba la inicialización del pin físico 
// por el que sale la señal PWM y del timer1 que la genera
void initpwm(void){
    initPINOUT();
    initTIM1(); // Cambiado a TIM1 (con respecto a P2)
}


//Configuración del pin PE9 (TIM1_CH1)
void initPINOUT(void){
    GPIO_InitTypeDef GPIO_InitStructurePINOUT = {0};
    
    __HAL_RCC_GPIOE_CLK_ENABLE(); // Reloj del puerto E habilitado

    GPIO_InitStructurePINOUT.Mode = GPIO_MODE_AF_PP;      
    GPIO_InitStructurePINOUT.Pull = GPIO_NOPULL;
    GPIO_InitStructurePINOUT.Speed = GPIO_SPEED_FREQ_LOW;
    
    // --- CAMBIO IMPORTANTE ---
    // PE9 usa AF1 para TIM1.
    // Aunque el nombre de la constante suele ser GPIO_AF1_TIM1, 
    // coincide en valor con GPIO_AF1_TIM2, pero lógicamente estamos conectando al TIM1.
    GPIO_InitStructurePINOUT.Alternate = GPIO_AF1_TIM1;  
    GPIO_InitStructurePINOUT.Pin = GPIO_PIN_9; 

    HAL_GPIO_Init(GPIOE, &GPIO_InitStructurePINOUT);
}

// Inicialización del Timer 1
void initTIM1(void){
      
    //TIM_OC_InitTypeDef TIM_Channel_InitStruct;
  
    //Instanciamos los valores del Timer
    htim1.Instance = TIM1;
    
    // --- AJUSTE DE RELOJ ---
    // TIM1 está en el bus APB2 (High Speed), que va al doble de velocidad que TIM2 (APB1).
    // Si antes usabas Prescaler 999 para 84MHz -> 84kHz.
    // Ahora con 168MHz/180MHz, usamos el doble (1999) para mantener el mismo tono.
    htim1.Init.Prescaler = 1999; 
    htim1.Init.Period = 104; 
    //htim1.Init.RepetitionCounter = 0; // Necesario para TIM1 (Timer Avanzado)

    // Habilitamos el reloj TIM1
    __HAL_RCC_TIM1_CLK_ENABLE();
  
    //Configuración del Timer y orden de inicio
    HAL_TIM_PWM_Init(&htim1);
  
    // Configuramos el Canal 1 (TIM1_CH1)
    TIM_Channel_InitStruct.OCMode = TIM_OCMODE_PWM1;        
    TIM_Channel_InitStruct.OCPolarity = TIM_OCPOLARITY_HIGH;  
    TIM_Channel_InitStruct.OCFastMode = TIM_OCFAST_DISABLE;
    //TIM_Channel_InitStruct.OCNPolarity = TIM_OCNPOLARITY_HIGH; // Necesario init para TIM1
    TIM_Channel_InitStruct.Pulse = 52;                         
    
    // Usamos TIM_CHANNEL_1
    HAL_TIM_PWM_ConfigChannel(&htim1, &TIM_Channel_InitStruct, TIM_CHANNEL_1);
}

/*********************************************************************/
/************      HILO PARA PROBAR EL FUNCIONAMIENTO     ************/
/*********************************************************************/

////Función de iniciaclización del hilo del test de prueba
int Init_Test_PWM(void) {
    tid_TestPWM = osThreadNew(TestPWM, NULL, NULL);
    if (tid_TestPWM == NULL) {
        return -1;
    }   
    return 0;
}

//Función del hilo del test de prueba
void TestPWM(void *argument) {
    uint8_t msg;
    
    while (1) {
        // 1. Prueba del pitido de PLAY/PAUSE
        msg = 0;
        osMessageQueuePut(IdcolaPWM, &msg, NULL, 0);
        osDelay(2000); 
        
        // 2. Prueba del pitido de SD no detectada
        msg = 1;
        osMessageQueuePut(IdcolaPWM, &msg, NULL, 0);
        osDelay(5000); 
    }
}