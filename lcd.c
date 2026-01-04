/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión del LCD
  */
  

/* Includes ------------------------------------------------------------------*/
#include "lcd.h"
#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "Arial12x12.h"

#define MSGQUEUE_OBJECTS 16 //Define el tamaño de la cola

/*----------------------------------------------------------------------------
 *      Thread 1 'lcd': Hilo del LCD
 *---------------------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static GPIO_InitTypeDef GPIO_InitStructurePIN; //Creamos la estructura para configurar el pin 

//Variables necesarias para la comunicación SPI
extern ARM_DRIVER_SPI Driver_SPI1;
ARM_DRIVER_SPI* SPIdrv = &Driver_SPI1;

osMessageQueueId_t IdcolaLCD;      //Declaración del id de la cola

infoLCD datosLCD;

//Añadido en P3_4
static uint16_t buffer[512];       //IMPORTANTE: con uint8_t no da para todo el buffer (2^8 = 256)

//Añadido en P3_5
static uint16_t positionL1 = 0;    //Línea 1 contiene las 2 primeras páginas: 0 a 127 y 128 a 255
//Añadido en P3_6
static uint16_t positionL2 = 256;  //Línea 1 contiene las 2 últimas páginas: 256 a 383 y 384 a 511

static uint32_t valor1 = 487;
static float valor2 = 2.65936f; 
//static char frase1[80];   //Cadena de caracteres en la que se copiará y desde la que se escribirá la info de la línea 1
//static char frase2[80];   //Cadena de caracteres en la que se copiará y desde la que se escribirá la info de la línea 2

osThreadId_t tid_ThLCD;          // Id del hilo

uint8_t valorJoystick = 0;       //Variable donde almacenamos lo leído por la cola

//Función de inicialización del hilo del LCD
int Init_LCD (void) {
  
  IdcolaLCD = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(infoLCD), NULL);   //Creamos la cola y la asignamos al Id correspondiente
 
	//Creamos e iniciamos un nuevo hilo que asignamos al identificador del ThtimerRebo,
	// le pasamos como parámetros la función que ejecutará el hilo
  tid_ThLCD = osThreadNew(LCD, NULL, NULL);
  if (tid_ThLCD == NULL) {
    return(-1);
  }
 
  return(0);
}

//Función del hilo del LCD
void LCD (void *argument){
  
    LCD_reset();  //Reseteo del LCD
    LCD_init();   //Inicio del LCD

    LCD_update(); //Actualización del LCD
  
    //escribirModoReposo();
    //escribirModoReproduccion();
    //escribirModoProgramacion();

    //LCD_update(); //Actualización del LCD


  while(1){
    
    //Leemos de la cola y guardamos en la variable datosLCD, esperamos infinitamente hasta que se introduzca en ella un nuevo elemento
    osMessageQueueGet(IdcolaLCD, &datosLCD, NULL, osWaitForever);
    
    borrarBuffer();
		
		escribirLCD(datosLCD.frase1, datosLCD.frase2);
    
    dibujarSubrayado(datosLCD.posCursor);
    
		LCD_update(); //Actualización del LCD

//    borrarBuffer();     //Limpieza del buffer (ponemos todos sus elementos a 0 y reseteamos la posición de las líneas)
//		sentenceToLocalBuffer(1, "     Pulsacion");
//    
//    //Dependiendo del valor leído en valorJoystick, escribimos en el LCD el gesto correspondiente
//    if(valorJoystick == 2){
//      sentenceToLocalBuffer(2, "     DERECHA");
//    }
//    else if(valorJoystick == 8){
//      sentenceToLocalBuffer(2, "     IZQUIERDA");
//    }
//    else if(valorJoystick == 1){
//      sentenceToLocalBuffer(2, "      ARRIBA");
//    }
//    else if(valorJoystick == 4){
//      sentenceToLocalBuffer(2, "      ABAJO");
//    }
//    else if(valorJoystick == 16){
//      sentenceToLocalBuffer(2, "     CENTRAL");
//    }
//    
    //LCD_update(); //Actualización del LCD
  }
}


//Inicialización del PIN RESET del SPI
void initpinesLCD(void){	
    
    //Inicialización del pin del Reset
    __HAL_RCC_GPIOA_CLK_ENABLE(); //Habilitamos el pin (está en el PA6)
    
    GPIO_InitStructurePIN.Mode = GPIO_MODE_OUTPUT_PP; //Configuramos el modo en pull-push salida
    GPIO_InitStructurePIN.Pin = GPIO_PIN_6; //Asignamos el PIN_6 del GPIO que le digamos después
    
    // relacionamos la configuración anterior con el GPIO correspondiente e inicializamos,
    // resultando en que estamos configurando el PIN_6 del bus A (PA6)
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructurePIN); 
    
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);    //Nos aseguramos de que empiece a nivel alto

    
    //Inicialización del pin del CS
    __HAL_RCC_GPIOD_CLK_ENABLE(); //Habilitamos el pin (está en el PD14)
    
    GPIO_InitStructurePIN.Mode = GPIO_MODE_OUTPUT_PP; //Configuramos el modo en pull-push salida
    GPIO_InitStructurePIN.Pin = GPIO_PIN_14; //Asignamos el PIN_14 del GPIO que le digamos después
    
    // relacionamos la configuración anterior con el GPIO correspondiente e inicializamos,
    // resultando en que estamos configurando el PIN_14 del bus D (PD14)
    HAL_GPIO_Init(GPIOD, &GPIO_InitStructurePIN); 
    
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);   //Nos aseguramos de que empiece a nivel alto

    
    //Inicialización del pin del A0
    __HAL_RCC_GPIOF_CLK_ENABLE(); //Habilitamos el pin (está en el PF13)
    
    GPIO_InitStructurePIN.Mode = GPIO_MODE_OUTPUT_PP; //Configuramos el modo en pull-push salida
    GPIO_InitStructurePIN.Pin = GPIO_PIN_13; //Asignamos el PIN_13 del GPIO que le digamos después
    
    // relacionamos la configuración anterior con el GPIO correspondiente e inicializamos,
    // resultando en que estamos configurando el PIN_13 del bus F (PF13)
    HAL_GPIO_Init(GPIOF, &GPIO_InitStructurePIN);
    
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, 1);   //Nos aseguramos de que empiece a nivel alto
    
}

//Función de inicialización del SPI
void SPI_Init(void){
  
    //Inicialización y configuración del SPI para realizar la gestión del LCD
    SPIdrv -> Initialize(NULL);
    SPIdrv -> PowerControl(ARM_POWER_FULL); //Ponemos el Power en ON

    //Configuramos parámetros de control: spi trabajando en modo máster, 
    // con configuración de inicio de transmisión CPOL a 1 y CPHA a 1,
    // organización de la información de most significant bit a least significant 
    // y número de bits por dato a 8.
    // Por último, la frecuencia del sclk a 20MHz
    SPIdrv -> Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL1_CPHA1 | ARM_SPI_MSB_LSB | ARM_SPI_DATA_BITS(8), 1000000 );

    //En este caso configuramos el control de slaves de este SPI y decimos que está inactivo
    SPIdrv -> Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
}

//Función de reset del LCD
void LCD_reset(void){

    initpinesLCD(); //Inicializamos los 3 pines GPIO que usa el LCD (RESET, CS, A0)
    SPI_Init();     //Inicializamos la comunicación SPI

    //Generamos la señal de reset solicitada
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
    osDelay(1);     //Esperamos 1 ms (realmente el reste necesita sólo 1 us pero el mínimo de HAL_Delay es 1 ms)
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
    osDelay(1);     //Esperamos 1 ms para poder empezar a trabajar con el LCD
	
}

/************************************ AÑADIDO P3 - EJ 2 ****************************************/

//Función para escribir datos en el LCD 
void LCD_wr_data(unsigned char data){

    // Seleccionar CS = 0;
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
    // Seleccionar A0 = 1;
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, 1);

    // Escribir un dato (data) usando la función SPIDrv->Send(…);
    SPIdrv -> Send(&data, sizeof(data));
    // Esperar a que se libere el bus SPI;
    while(SPIdrv -> GetStatus().busy);
    
    // Seleccionar CS = 1;
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);

}

//Función para escribir comandos en el LCD
void LCD_wr_cmd(unsigned char cmd){
    
    // Seleccionar CS = 0;
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
    // Seleccionar A0 = 0;
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, 0);
    
    // Escribir un comando (cmd) usando la función SPIDrv->Send(…);
    SPIdrv -> Send(&cmd, sizeof(cmd));
    // Esperar a que se libere el bus SPI;
    while(SPIdrv -> GetStatus().busy);
    
    // Seleccionar CS = 1;
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);
}

/************************************ AÑADIDO P3 - EJ 3 ****************************************/

//Función Inicializar el LCD
void LCD_init(void){

    LCD_wr_cmd(0xAE); //Pone el display a OFF
    
    LCD_wr_cmd(0xA2); //Fija el valor de la relación de la tensión de polarización del LCD a 1/9 

    LCD_wr_cmd(0xA0); //Pone el direccionamiento de la RAM de datos del display en normal

    LCD_wr_cmd(0xC8); //Pone el scan en las salidas COM en normal

    LCD_wr_cmd(0x22); //Fija la relación de resistencias interna a 2

    LCD_wr_cmd(0x2F); //Se activa el power a ON

    LCD_wr_cmd(0x40); //El display empieza en la línea 0

    LCD_wr_cmd(0xAF); //El display se pone a ON

    LCD_wr_cmd(0x81); //Se retoca el contraste

    LCD_wr_cmd(0x10); //Se decide el valor del contraste (a elegir)

    LCD_wr_cmd(0xA4); //Se ponen todos los puntos del display en normal

    LCD_wr_cmd(0xA6); //Se pone el LCD de display en normal

}
    
    
/**************************** AÑADIDO P3 - EJ 4 ********************************/

//Función para actualizar la representación del LCD
void LCD_update(void){
  
    int i;
    LCD_wr_cmd(0x00); // 4 bits de la parte baja de la dirección a 0
    LCD_wr_cmd(0x10); // 4 bits de la parte alta de la dirección a 0
    LCD_wr_cmd(0xB0); // Página 0
    for(i=0;i<128;i++){
      LCD_wr_data(buffer[i]);
    }
    
    LCD_wr_cmd(0x00); // 4 bits de la parte baja de la dirección a 0
    LCD_wr_cmd(0x10); // 4 bits de la parte alta de la dirección a 0
    LCD_wr_cmd(0xB1); // Página 1
    for(i=128;i<256;i++){
      LCD_wr_data(buffer[i]);
    }
    
    LCD_wr_cmd(0x00);
    LCD_wr_cmd(0x10);
    LCD_wr_cmd(0xB2); //Página 2
    for(i=256;i<384;i++){
      LCD_wr_data(buffer[i]);
    }
    
    LCD_wr_cmd(0x00);
    LCD_wr_cmd(0x10);
    LCD_wr_cmd(0xB3); // Pagina 3
    for(i=384;i<512;i++){
      LCD_wr_data(buffer[i]);
    }
}


//Función para pintar fácil en una columna del LCD
void pintarColumnaLCD(uint8_t columna, uint8_t pagina, uint8_t valor){ 

    buffer[columna + pagina * 128] = valor;
}


/**************************** AÑADIDO P3 - EJ 5 ********************************/

//Función para escribir un símbolo en la línea 1 del LCD
void symbolToLocalBuffer_L1(uint8_t symbol){

    uint8_t i, value1, value2;
    uint16_t offset = 0;

    offset = 25*(symbol - ' ');

    for (i=0; i<12; i++){
      value1 = Arial12x12[offset + i*2 + 1];
      value2 = Arial12x12[offset + i*2 + 2];

      buffer[i + positionL1] = value1;
      buffer[i + 128 + positionL1] = value2;
    }
    
    positionL1 = positionL1 + Arial12x12[offset];
}

//Función para escribir una frase en la línea 1 del LCD
void sentenceToLocalBuffer_L1(const char *frase){

    uint8_t i, value1, value2;
    uint16_t offset = 0;

    for(size_t j=0; j<strlen(frase); j++){
      char symbol = frase[j];
      offset = 25*(symbol - ' ');

      for (i=0; i<12; i++){
        value1 = Arial12x12[offset + i*2 + 1];
        value2 = Arial12x12[offset + i*2 + 2];

        buffer[i + positionL1] = value1;
        buffer[i + 128 + positionL1] = value2;
      }
      
    positionL1 = positionL1 + Arial12x12[offset];
    }
}


/**************************** AÑADIDO P3 - EJ 6 ********************************/

//Función para escribir un símbolo en la línea 2 del LCD
void symbolToLocalBuffer_L2(uint8_t symbol){

    uint8_t i, value1, value2;
    uint16_t offset = 0;

    offset = 25*(symbol - ' ');

    for (i=0; i<12; i++){
      value1 = Arial12x12[offset + i*2 + 1];
      value2 = Arial12x12[offset + i*2 + 2];

      buffer[i + positionL2] = value1;
      buffer[i + 128 + positionL2] = value2;
    }
    
    positionL2 = positionL2 + Arial12x12[offset];
}

//Función para escribir una frase en la línea 2 del LCD
void sentenceToLocalBuffer_L2(const char *frase){

    uint8_t i, value1, value2;
    uint16_t offset = 0;

    for(size_t j=0; j<strlen(frase); j++){
      char symbol = frase[j];
      offset = 25*(symbol - ' ');

      for (i=0; i<12; i++){
        value1 = Arial12x12[offset + i*2 + 1];
        value2 = Arial12x12[offset + i*2 + 2];

        buffer[i + positionL2] = value1;
        buffer[i + 128 + positionL2] = value2;
      }
      
    positionL2 = positionL2 + Arial12x12[offset];
    }
}

//Función para escribir un símbolo en la línea 2 del LCD
void symbolToLocalBuffer(uint8_t linea, uint8_t symbol){
    if(linea == 1){
      symbolToLocalBuffer_L1(symbol);
    } else if(linea == 2){
      symbolToLocalBuffer_L2(symbol);
    }
}

//Función para escribir una frase en la línea 2 del LCD
void sentenceToLocalBuffer(uint8_t linea, const char *frase){
    if(linea == 1){
      sentenceToLocalBuffer_L1(frase);
    } else if(linea == 2){
      sentenceToLocalBuffer_L2(frase);
    }
}

//Función para escribir una oración cualquiera en las líneas que se digan del LCD (contemplando también variables, no solo texto)
void escribirLCD(char frase1[80], char frase2[80]){

//    sprintf(frase1, "Prueba valor1: %d", valor1);     //Frase 1 que se escribirá por pantalla
//    sprintf(frase2, "Prueba valor2: %.5f", valor2);   //Frase 2 que se escribirá por pantalla

    //Bucle for que recorre cada caracter de la frase 1, hasta encontrar un caracter vacío
    // escribiéndola caracter a caracter en la línea 1 mediante la función symbolToLocalBuffer
    for(int i = 0; frase1[i] != '\0'; i++){
      symbolToLocalBuffer(1,frase1[i]);
    }

    //Bucle for que recorre cada caracter de la frase 2,  hasta encontrar un caracter vacío
    // escribiéndola caracter a caracter en la línea 2 mediante la función symbolToLocalBuffer
    for(int j = 0; frase2[j] != '\0'; j++){
      symbolToLocalBuffer(2,frase2[j]);
    }
}

/**************************** AÑADIDO P5 - EJ 3 ********************************/
//Función para limpiar el buffer cómodamente
void borrarBuffer(void){
  
    //Reseteamos las posiciones de L1 y L2 a sus valores iniciales
    positionL1 = 0;
    positionL2 = 256;
  
    //Bucle for que recorre los 512 elementos del buffer poniendo a 0 todos ellos
    for(int j = 0; j < 512; j++){
      buffer[j] = 0x00;
    }
}

/************************************************************************************/
/************************** FUNCIONES ADICIONALES PROYECTO **************************/
/************************************************************************************/

//Función que escribe la pantalla tipo de modo REPOSO
void escribirModoReposo(char frase1[80], char frase2[80]){

    sprintf(frase1, "SBM 2025  Ta 25.5*C");       //Frase 1 que se escribirá por pantalla
    sprintf(frase2, "      16:05:00");            //Frase 2 que se escribirá por pantalla

    //Bucle for que recorre cada caracter de la frase 1, hasta encontrar un caracter vacío
    // escribiéndola caracter a caracter en la línea 1 mediante la función symbolToLocalBuffer
    for(int i = 0; frase1[i] != '\0'; i++){
      symbolToLocalBuffer(1,frase1[i]);
    }

    //Bucle for que recorre cada caracter de la frase 2,  hasta encontrar un caracter vacío
    // escribiéndola caracter a caracter en la línea 2 mediante la función symbolToLocalBuffer
    for(int j = 0; frase2[j] != '\0'; j++){
      symbolToLocalBuffer(2,frase2[j]);
    }
}

//Función que escribe la pantalla tipo de modo REPRODUCCIÓN
void escribirModoReproduccion(char frase1[80], char frase2[80]){

    sprintf(frase1, "F:XX C:YY    Vol: ZZ");      //Frase 1 que se escribirá por pantalla
    sprintf(frase2, "      T: 00:23");            //Frase 2 que se escribirá por pantalla

    //Bucle for que recorre cada caracter de la frase 1, hasta encontrar un caracter vacío
    // escribiéndola caracter a caracter en la línea 1 mediante la función symbolToLocalBuffer
    for(int i = 0; frase1[i] != '\0'; i++){
      symbolToLocalBuffer(1,frase1[i]);
    }

    //Bucle for que recorre cada caracter de la frase 2,  hasta encontrar un caracter vacío
    // escribiéndola caracter a caracter en la línea 2 mediante la función symbolToLocalBuffer
    for(int j = 0; frase2[j] != '\0'; j++){
      symbolToLocalBuffer(2,frase2[j]);
    }
}

//Función que escribe la pantalla tipo del modo PROGRAMACIÓN DE LA HORA
void escribirModoProgramacion(char frase1[80], char frase2[80]){

    sprintf(frase1, " HORA  Ta 25.5 *C");         //Frase 1 que se escribirá por pantalla
    sprintf(frase2, "      16:05:00");            //Frase 2 que se escribirá por pantalla

    //Bucle for que recorre cada caracter de la frase 1, hasta encontrar un caracter vacío
    // escribiéndola caracter a caracter en la línea 1 mediante la función symbolToLocalBuffer
    for(int i = 0; frase1[i] != '\0'; i++){
      symbolToLocalBuffer(1,frase1[i]);
    }

    //Bucle for que recorre cada caracter de la frase 2,  hasta encontrar un caracter vacío
    // escribiéndola caracter a caracter en la línea 2 mediante la función symbolToLocalBuffer
    for(int j = 0; frase2[j] != '\0'; j++){
      symbolToLocalBuffer(2,frase2[j]);
    }
}


// Función auxiliar para dibujar la barra en PROGRAMACIÓN DE HORA
void dibujarSubrayado(int8_t pos) {
    if (pos < 0 || pos > 5) return; // Si es -1 o inválido, no hacemos nada

    uint16_t startX = 0;
    
    // Primero calculamos el offset inicial por los 6 espacios del principio
    // 6 espacios * 7 pixeles de ancho = 42 pixeles
    uint16_t offsetBase = 42; 

    // Calculamos la posición X exacta según el dígito
    switch(pos) {
        case 0: // Decenas Horas
            startX = offsetBase;
        break;
        case 1: // Unidades Horas (6px del digito anterior)
            startX = offsetBase + 6;
        break;
        
        case 2: // Decenas Minutos (12px horas + 2px dos puntos)
            startX = offsetBase + 12 + 2;
        break;
        
        case 3: // Unidades Minutos
            startX = offsetBase + 18 + 2;
        break;
        
        case 4: // Decenas Seg (24px prev + 4px de dos "dos puntos")
            startX = offsetBase + 24 + 4; 
        break;
        
        case 5: // Unidades Segundos
            startX = offsetBase + 30 + 4;
        break;
    }

    // Dibujamos la línea en el buffer
      // La línea 2 del texto ocupa las páginas 2 y 3 del buffer.
      // La parte baja del texto está en la página 3.
      // El buffer de la página 3 empieza en el índice 384 (128*3).
    
    for (int i = 0; i < 6; i++) { // El ancho del número es 6 pixeles
        // Index = Inicio Pag 3 + Coordenada X calculada + iterador
        int index = 384 + startX + i; 
        
        if(index < 512) {
            // Pintamos la línea 4 píxeles por encima del final, para que se vea cerca del número 
            buffer[index] |= 0x08; 
        }
    }
}
