#include "cmsis_os2.h"
#include "rgb.h"

osThreadId_t tid_ThledsRGB;
osMessageQueueId_t IdColaRGB;

// Handler del Timer para el PWM
TIM_HandleTypeDef htim4;

int Init_ThledsRGB (void) {
  // Inicialización física (AHORA INCLUYE PWM)
  initLEDsRGB();
  
  // CAMBIO IMPORTANTE: El tamaño del elemento es sizeof(infoRGB)
  IdColaRGB = osMessageQueueNew(16, sizeof(infoRGB), NULL);
  
  tid_ThledsRGB = osThreadNew(ThledsRGB, NULL, NULL);
  if (tid_ThledsRGB == NULL) return(-1);
  return(0);
}

void ThledsRGB (void *argument) {
  osStatus_t status; 
  infoRGB msg; // AHORA RECIBIMOS LA ESTRUCTURA
  
  // Iniciamos el PWM
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);

  // Estado inicial apagado
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, 1);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 1);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);

  while (1) {
    status = osMessageQueueGet(IdColaRGB, &msg, NULL, 0);
    
    // Si no hay mensaje nuevo, mantenemos el estado anterior, 
    // pero si hubiera llegado uno nuevo, 'msg' ya tiene los datos frescos.
    // NOTA: Para que el parpadeo no se bloquee esperando cola, usamos timeout 0 
    // y una variable estática para recordar el estado si no llega nada nuevo.
    static infoRGB estadoActual = {RGB_OFF, 15};
    
    if (status == osOK) {
        estadoActual = msg;
    }

    switch(estadoActual.estado){
      case RGB_PLAY:
        // APAGAR OTROS
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, 1);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 1);

        // CÁLCULO DE BRILLO (0-30 -> 0-1000)
        uint32_t brillo = (estadoActual.volumen * 1000) / 30;

        // PARPADEO CON INTENSIDAD VARIABLE
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, brillo);
        osDelay(125);
        
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);
        osDelay(125);
        break;
      
      case RGB_PAUSE:
        // AZUL PARPADEANDO (GPIO)
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0); // Verde OFF
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 1);        // Rojo OFF
        
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_11);
        osDelay(500);
        break;
      
      case RGB_ERROR:
        // ROJO PARPADEANDO (GPIO)
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, 1);
        
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
        osDelay(100);
        break;
      
      default: // RGB_OFF
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, 1);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, 1);
        osDelay(500);
        break;
    }
  }
}

// Inicialización física de los pines RGB
  // (Pin correspondiente al VERDE inicializado como PWM)
void initLEDsRGB(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE(); // Reloj del Timer 4, para el PWM

    // Configuración leds ROJO y AZUL (GPIOs normales)
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Configuración led VERDE (PD12) como PWM
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; 
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM4; 
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Configuración del Timer 4
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 83; // Ajustar según tu reloj (para ~1MHz)
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 1000;  // Periodo PWM
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim4);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    // POLARIDAD LOW: Como el LED se enciende con '0', usamos polaridad invertida
    // para que un valor alto de PWM signifique "más encendido".
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW; 
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1);
}
