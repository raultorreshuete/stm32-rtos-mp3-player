/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión de la comunicación de la placa con el PC 
    (mediante tramas UART con Teraterm)
  */
	
#ifndef _COM_
#define _COM_

	#include "stm32f4xx_hal.h"
	#include "Driver_USART.h"
	
	#define TRAMA_TRANSMITIDA 0x00000001U
	
		// ***** Bytes de Comando que se envía al MP3 (4o byte de la trama) *****
	#define CMD_NEXT_SONG     0x01    //Pasar a la siguiente canción en la carpeta
	#define CMD_PREV_SONG     0x02    //Retroceder a la anterior canción en la carpeta
	#define CMD_PLAY_INDEX    0x03    //Reproducir canción mediante su posición en la carpeta
	#define CMD_VOL_UP        0x04    //Subir en 1 el volumen
	#define CMD_VOL_DOWN      0x05    //Bajar en 1 el volumen
	#define CMD_SET_VOL       0x06    //Poner el volumen al valor deseado
	#define CMD_SEL_DEV       0x09    //Elegir localización de la memoria (elegir dispositivo)
	#define CMD_RESET         0x0C    //Reset del chip
	#define CMD_PLAY          0x0D    //Continuar reproduciendo la canción actual (pausada previamente)
	#define CMD_PAUSE         0x0E    //Pausar la canción actual
	#define CMD_PLAY_FOLDER   0x0F    //Reproducir canción por su ruta (nº carpeta y nºo canción en esa carpeta)
	#define CMD_STOP          0x16    //Parar la reproducción
  
    // **********         Bytes de COMANDO que se enían al MP3 para consultar info         **********
  // **********     (Entrega la info en el 7o Byte, data2 del struct ComandoCompletoMP3) **********
  // **********                (y se consulta mediante la callback del UART)             **********
  #define CMD_INFO_STATUS       0x42    //Informa de el estado del MP3 (0 si STOP, 1 si PLAYING y 2 si PAUSE
  #define CMD_INFO_VOL          0x43    //Informa del volumen actual del MP3
  #define CMD_INFO_ARCHIVOS     0x48    //Manda cuántas canciones hay en la sd (es en total, no por carpeta)
  #define CMD_INFO_CARPETAS     0x4F    //Manda cuántas carpetas hay en la sd
	
	#define MSGQUEUE_OBJECTS_COM 16
	
		int Init_Com(void); //Función de inicialización del hilo

		int Init_Test_Com(void); //Función de inicialización del hilo de prueba

		void ThComTrans(void *argument);
    

//		// Definimos la estructura de la trama para la cola de mensajes
//		typedef struct {
//				uint8_t cmd;      // CMD (4o byte de la trama)
//				uint8_t feedback; // 5o byte
//				uint8_t data1;    // MSB del dato (6o byte)
//				uint8_t data2;    // LSB del dato (7o byte)
//		} ComandoCompletoMP3;
		
#endif