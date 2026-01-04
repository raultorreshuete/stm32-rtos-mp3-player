/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión del joystick y sus rebotes
  */

#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "joystick.h"

#define MSGQUEUE_OBJECTS 1 //Define el tamaño de la cola

/*---------------------------------------------------------------------------------------------
 *		Threads 2 y 3 'joystick' y 'ThtimerRebo': Hilos del joystick y del timer de los rebotes
 *---------------------------------------------------------------------------------------------*/
 
osThreadId_t tid_joystick;            //Id del hilo del joystick
osMessageQueueId_t IdcolaJoystick; 		//Declaración del id de la cola

osTimerId_t tid_tim1segR; 							//Id del timer de 1 seg (gestiona tipo pulsación
osTimerId_t tid_tim1segC; 							//Id del timer de 1 seg (gestiona tipo pulsación
osTimerId_t tid_tim1segL; 							//Id del timer de 1 seg (gestiona tipo pulsación


infoJoys datosJoys;										//Estructura que metemos en la cola, contiene el PIN pulsado y si la pulsación es larga o corta
uint32_t exec1 = 1U; 									//Argumento para la funcion callback del timer

//Variables para depuración de las pulsaciones de cada gesto
uint8_t pulsadito_right, pulsadito_left, pulsadito_up, pulsadito_down, pulsadito_middle, pulsadito_largo;

//Id's relacionados con el timer de los rebotes
osThreadId_t tid_ThtimerRebo;     		//Id del hilo del timer de los rebotes
osTimerId_t tid_timRebo; 							//Id del timer de los rebotes


//Función de inicialización del hilo del joystick
int Init_joystick (void) {
  
  initJoys();   //Llamamos a la inicialización física del joystick
  
  IdcolaJoystick = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(infoJoys), NULL);
  
  tid_joystick = osThreadNew(joystick, NULL, NULL);
  if (tid_joystick == NULL) {
    return(-1);
  }
 
  return(0);
}
  
//Función del hilo del joystick
void joystick (void *argument) {
   
  while (1) {
    
    //Esperamos a que nos llegue el flag del timer de los rebotes
    osThreadFlagsWait(TIMER_REBO, osFlagsWaitAll, osWaitForever);
        
    if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11)){ 								// Si el pin leído corresponde al RIGHT
      tid_tim1segR = osTimerNew((osTimerFunc_t)&Callback_timer1segR, osTimerOnce, &exec1, NULL);		//Iniciamos un timer de 1 segundo para leer si la pulsación es larga o corta
      osTimerStart(tid_tim1segR, 1000U);
    }
    
    if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_14)){ 								// Si el pin leído corresponde al LEFT
      tid_tim1segL = osTimerNew((osTimerFunc_t)&Callback_timer1segL, osTimerOnce, &exec1, NULL);		//Iniciamos un timer de 1 segundo para leer si la pulsación es larga o corta
      osTimerStart(tid_tim1segL, 1000U);
    }

    if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10)){ 								// Si el pin leído corresponde al UP
      datosJoys.GPIO_PIN = GPIO_PIN_10;
      datosJoys.tipo_pulsacion = 0;
      osMessageQueuePut(IdcolaJoystick, &datosJoys, NULL, 0);    //Metemos en la cola que se ha pulsado UP y es pulsación corta
      pulsadito_up++;
    }

    if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_12)){ 								// Si el pin leído corresponde al DOWN
      datosJoys.GPIO_PIN = GPIO_PIN_12;
      datosJoys.tipo_pulsacion = 0;
      osMessageQueuePut(IdcolaJoystick, &datosJoys, NULL, 0);    //Metemos en la cola que se ha pulsado DOWN y es pulsación corta
      pulsadito_down++;
    }

    if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_15)){ 								// Si el pin leído corresponde al MIDDLE
      tid_tim1segC = osTimerNew((osTimerFunc_t)&Callback_timer1segC, osTimerOnce, &exec1, NULL);		//Iniciamos un timer de 1 segundo para leer si la pulsación es larga o corta
      osTimerStart(tid_tim1segC, 1000U);
      
    }
        
    osThreadYield();                            // suspend thread
  }
}
  
  
//Función Inicialización de las 5 acciones del Joystick
void initJoys(void){		
  
    GPIO_InitTypeDef GPIO_InitStructureJoys; //creamos la estructura para configurar los pines
  
    //Habilitamos los relojes de los puertos de los pines del joystick
		__HAL_RCC_GPIOB_CLK_ENABLE();
		__HAL_RCC_GPIOE_CLK_ENABLE();
  
    HAL_NVIC_EnableIRQ( EXTI15_10_IRQn );  //Activamos las interrupciones del joystick
	
		//Inicialización RIGHT
		GPIO_InitStructureJoys.Mode = GPIO_MODE_IT_RISING;
		GPIO_InitStructureJoys.Pull = GPIO_PULLDOWN;
		GPIO_InitStructureJoys.Pin = GPIO_PIN_11;
	
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructureJoys); 

		//Inicialización LEFT
		GPIO_InitStructureJoys.Mode = GPIO_MODE_IT_RISING;
		GPIO_InitStructureJoys.Pull = GPIO_PULLDOWN;
		GPIO_InitStructureJoys.Pin = GPIO_PIN_14;
	
    HAL_GPIO_Init(GPIOE, &GPIO_InitStructureJoys); 
	
		//Inicialización UP
		GPIO_InitStructureJoys.Mode = GPIO_MODE_IT_RISING;
		GPIO_InitStructureJoys.Pull = GPIO_PULLDOWN;
		GPIO_InitStructureJoys.Pin = GPIO_PIN_10;
	
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructureJoys); 
	
		//Inicialización DOWN
		GPIO_InitStructureJoys.Mode = GPIO_MODE_IT_RISING;
		GPIO_InitStructureJoys.Pull = GPIO_PULLDOWN;
		GPIO_InitStructureJoys.Pin = GPIO_PIN_12;
	
    HAL_GPIO_Init(GPIOE, &GPIO_InitStructureJoys); 
		
		//Inicialización MIDDLE
		GPIO_InitStructureJoys.Mode = GPIO_MODE_IT_RISING;
		GPIO_InitStructureJoys.Pull = GPIO_PULLDOWN;
		GPIO_InitStructureJoys.Pin = GPIO_PIN_15;
	
    HAL_GPIO_Init(GPIOE, &GPIO_InitStructureJoys); 
	}

  
//Implementación para gestionar interrupciones de la acción RIGHT del Joystick
void EXTI15_10_IRQHandler(void){
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);

}
        
//Configuración del callback, la rutina que seguirá el programa cada vez que haya una interrupción
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_PIN){ 
  
    osThreadFlagsSet(tid_ThtimerRebo, JOYS_PULSADO); //Enviamos al hilo deL timer de los rebotes el flag de que se ha pulsado el joystick
}

// Callback del timer de 1 segundo del botón MIDDLE
void Callback_timer1segC (void const *arg) {

  if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_15)){ //Si el pin leído después de que venza el timer sigue siendo MIDDLE
    datosJoys.GPIO_PIN = GPIO_PIN_15;
    datosJoys.tipo_pulsacion = 1;						//Activamos la variable que indica que es pulsación larga
    pulsadito_largo++;

  }
  else{																			// Si, en cambio, ya no se está pulsando MIDDLE
    datosJoys.GPIO_PIN = GPIO_PIN_15;				
    datosJoys.tipo_pulsacion = 0;						//Mantenemos pulsación corta
    pulsadito_middle++;
  }

  osMessageQueuePut(IdcolaJoystick, &datosJoys, NULL, 0);    //Metemos en la cola que se ha pulsado MIDDLE y el tipo de pulsación que haya ocurrido
}

// Callback del timer de 1 segundo del botón RIGHT
void Callback_timer1segR (void const *arg) {

  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11)){ //Si el pin leído después de que venza el timer sigue siendo RIGHT
    datosJoys.GPIO_PIN = GPIO_PIN_11;
    datosJoys.tipo_pulsacion = 1;						//Activamos la variable que indica que es pulsación larga
    pulsadito_largo++;

  }
  else{																			// Si, en cambio, ya no se está pulsando RIGHT
    datosJoys.GPIO_PIN = GPIO_PIN_11;				
    datosJoys.tipo_pulsacion = 0;						//Mantenemos pulsación corta
    pulsadito_middle++;
  }

  osMessageQueuePut(IdcolaJoystick, &datosJoys, NULL, 0);    //Metemos en la cola que se ha pulsado RIGHT y el tipo de pulsación que haya ocurrido
}

// Callback del timer de 1 segundo del botón LEFT
void Callback_timer1segL (void const *arg) {

  if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_14)){ //Si el pin leído después de que venza el timer sigue siendo LEFT
    datosJoys.GPIO_PIN = GPIO_PIN_14;
    datosJoys.tipo_pulsacion = 1;						//Activamos la variable que indica que es pulsación larga
    pulsadito_largo++;

  }
  else{																			// Si, en cambio, ya no se está pulsando LEFT
    datosJoys.GPIO_PIN = GPIO_PIN_14;				
    datosJoys.tipo_pulsacion = 0;						//Mantenemos pulsación corta
    pulsadito_middle++;
  }

  osMessageQueuePut(IdcolaJoystick, &datosJoys, NULL, 0);    //Metemos en la cola que se ha pulsado LEFT y el tipo de pulsación que haya ocurrido
}

//*****************************************************************************************************
//**************************** HILO DEL TIMER QUE GESTIONA LOS REBOTES ********************************
//*****************************************************************************************************

int Init_ThtimerRebo (void) {
 
	//Creamos e iniciamos un nuevo hilo que asignamos al identificador del ThtimerRebo,
	// le pasamos como parámetros la función que ejecutará el hilo
  tid_ThtimerRebo = osThreadNew(ThtimerRebo, NULL, NULL);
  if (tid_ThtimerRebo == NULL) {
    return(-1);
  }
 
  return(0);
}
 
void ThtimerRebo (void *argument){
  	
	osThreadFlagsWait(JOYS_PULSADO, osFlagsWaitAll, osWaitForever);   //Esperamos hasta que nos llegue el flag que nos indica que se ha pulsado el joystick
	
  //Crea un timer one-shoot (que solo se ejecutará una vez, no periódicamente)
  uint32_t exec2 = 1U; //Argumento para la funcion callback del timer
  tid_timRebo = osTimerNew((osTimerFunc_t)&Callback_timerRebo, osTimerOnce, &exec2, NULL);
  if (tid_timRebo != NULL) {  //One-shot timer creado
    // Empieza el timer
    osTimerStart(tid_timRebo, 100U);
  }

  while(1){
		
		//Espera al flag que indica que se ha pulsado el joystick
    osThreadFlagsWait(JOYS_PULSADO, osFlagsWaitAll, osWaitForever); 
		//Resetea el timer
    osTimerStart(tid_timRebo, 100U);
  }
}

void Callback_timerRebo (void const *arg) {
	
	osThreadFlagsSet(tid_joystick, TIMER_REBO); //Manda al joystick la flag que indica que el timer ya ha finalizado
	
}

