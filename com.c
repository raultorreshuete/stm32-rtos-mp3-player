/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión de la comunicación de la placa con el PC 
    (mediante tramas UART con Teraterm)
  */

#include "cmsis_os2.h"                          
#include "com.h"
//#include "clock.h"                      // <--- IMPORTANTE: Incluimos el reloj
#include "mp3.h"
#include "stm32f4xx.h"
#include "Driver_I2C.h"                 
#include <stdio.h>
#include <string.h> 

#define MSGQUEUE_OBJECTS 16

// ***** Bytes siempre constantes de cada trama al MP3 *****
static const uint8_t MP3_START = 0x7E;    //Byte inicial de cada trama
static const uint8_t MP3_VER   = 0xFF;    //Byte de versión que lleva cada trama (2o byte)
static const uint8_t MP3_LEN   = 0x06;    //Byte de tamaño que lleva cada trama (3er byte)
static const uint8_t MP3_END   = 0xEF;    //Byte final de cada trama

osThreadId_t tid_comTrans;    //Id del hilo de transmisión por UART
osThreadId_t tid_ThTest;      //Id del hilo del test de prueba

osMessageQueueId_t IdqueueComTrans;     //Id de la cola del com-pc
ComandoCompletoMP3 mensajeRecibido;     //Estructura del comando del MP3

extern ARM_DRIVER_USART Driver_USART3;
static ARM_DRIVER_USART * USARTdrv = &Driver_USART3;

static char tramaTexto[60];     //Buffer donde metemos la info de la trama
                                //Formato: "HH:MM:SS---> 7E FF..."

// Prototipos locales
void Test_Com (void *argument);
void myUSART_callback(uint32_t event);
static int8_t Init_USART(void);

//Función inicialización del hilo del COM-PC
int Init_Com (void) {
    
    if(Init_USART() != 0) return -1;
    
    IdqueueComTrans = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(ComandoCompletoMP3), NULL);
    if (IdqueueComTrans == NULL) return -1;

    tid_comTrans = osThreadNew(ThComTrans, NULL, NULL);
    if (tid_comTrans == NULL) return -1;
 
    return 0;
}

//Función del hilo del COM-PC
void ThComTrans(void *argument) {
    
    while(1) {
        //Esperamos a que llegue una trama para enviar
        osMessageQueueGet(IdqueueComTrans, &mensajeRecibido, NULL, osWaitForever);
        
      //Formateamos con la hora del sistema y la trama:
      //  Guardamos en tramaTexto la info de las variables globales de hora, minuto y segundo
      //  y las guardamos con el formato pedido
      sprintf(tramaTexto, "%02d:%02d:%02d---> %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
              mensajeRecibido.hora, mensajeRecibido.minutos, mensajeRecibido.segundos,
              //Añadimos los bytes que componen la trama MP3 (los 3 primeros constantes)
              MP3_START,
              MP3_VER,
              MP3_LEN,
              //Estos 4 siguientes son los que indican el comando del MP3 (los que leemos de la cola)
              mensajeRecibido.cmd,
              mensajeRecibido.feedback,
              mensajeRecibido.data1,
              mensajeRecibido.data2,
              //Último byte constante de fin de trama
              MP3_END);
    
        //Enviamos la trama al PC
        USARTdrv->Send(tramaTexto, strlen(tramaTexto));
        
        //Esperamos confirmación de que se ha transmitido la trama
        osThreadFlagsWait(TRAMA_TRANSMITIDA, osFlagsWaitAll, osWaitForever);
    
        osThreadYield();
    }
}

//Función de inicialización del USART3
static int8_t Init_USART(void){
    // Inicializamos el USART driver, con su callbacsk
    if(USARTdrv->Initialize(myUSART_callback) != ARM_DRIVER_OK ||
       // Activamos el driver (lo enciende)
       USARTdrv->PowerControl(ARM_POWER_FULL) != ARM_DRIVER_OK ||
       //Configuramos el USART a 115200 Baudios (más cómodo para la lectura rápida por teraterm
       USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                          ARM_USART_DATA_BITS_8 |
                          ARM_USART_PARITY_NONE |
                          ARM_USART_STOP_BITS_1 |
                          ARM_USART_FLOW_CONTROL_NONE, 
                          115200) != ARM_DRIVER_OK ||
       //Habilitamos la tranmisión
       USARTdrv->Control (ARM_USART_CONTROL_TX, 1) != ARM_DRIVER_OK)
    {
        return -1;
    }
    // Desactivamos la recepción explícitamente para evitar errores
    USARTdrv->Control (ARM_USART_CONTROL_RX, 0);
    return 0;
}

//Función Callback del USART3
void myUSART_callback(uint32_t event){
    if (event & ARM_USART_EVENT_SEND_COMPLETE){   //Si todo ha ido bien
        //Mandamos la flag de que se ha completado la transmisión de la trama
        osThreadFlagsSet(tid_comTrans, TRAMA_TRANSMITIDA);
    }   
}

/*********************************************************************/
/************      HILO PARA PROBAR EL FUNCIONAMIENTO     ************/
/*********************************************************************/

//Función de iniciaclización del hilo del test de prueba
int Init_Test_Com(void) {
  tid_ThTest = osThreadNew(Test_Com, NULL, NULL);
  if (tid_ThTest == NULL) return -1;
  return 0;
}

//Función del hilo del test de prueba
void Test_Com (void *argument) {
    ComandoCompletoMP3 tramaPrueba;
    uint8_t fase = 0; // Para controlar la secuencia de pruebas

    while (1) {
        
        // Limpiamos parámetros por defecto
        tramaPrueba.feedback = 0x00;
        tramaPrueba.data1 = 0x00;
        tramaPrueba.data2 = 0x00;

        switch(fase) {
            case 0:
                // 1. SELECT DEVICE (TF Card = 0x02)
                tramaPrueba.cmd = CMD_SEL_DEV;
                tramaPrueba.data2 = 0x02; 
                break;
                
            case 1:
                // 2. SET VOLUMEN (Ej: 20 = 0x14)
                tramaPrueba.cmd = CMD_SET_VOL;
                tramaPrueba.data2 = 0x14; // Vol 20
                break;
                
            case 2:
                // 3. PLAY FOLDER (Carpeta 01, Canción 001)
                // Usamos 0x0F que es el comando robusto para estructura de ficheros
                tramaPrueba.cmd = CMD_PLAY_FOLDER;
                tramaPrueba.data1 = 0x01; // Carpeta 01
                tramaPrueba.data2 = 0x01; // Canción 001
                break;
                
            case 3:
                // 4. PAUSE
                tramaPrueba.cmd = CMD_PAUSE;
                break;

            case 4:
                // 5. RESUME (PLAY)
                tramaPrueba.cmd = CMD_PLAY;
                break;
                
            case 5:
                // 6. NEXT SONG
                tramaPrueba.cmd = CMD_NEXT_SONG;
                break;
                
            case 6:
                // 7. PREVIOUS SONG
                tramaPrueba.cmd = CMD_PREV_SONG;
                break;
                
            case 7:
                // 8. RESET (Para acabar el ciclo y ver que reinicia)
                tramaPrueba.cmd = CMD_RESET;
                break;
        }

        // Enviamos mensaje a la cola. 
        osMessageQueuePut(IdqueueComTrans, &tramaPrueba, NULL, 0);
        
        // Pasamos a la siguiente fase
        fase++;
        if(fase > 7) fase = 0; // Reiniciamos ciclo

        // Esperamos 3 segundos entre comandos para que te de tiempo a leer en TeraTerm
        osDelay(3000); 
    }
}