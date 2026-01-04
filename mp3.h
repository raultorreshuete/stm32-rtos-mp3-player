/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión del Reproductor MP3
  */

#ifndef MP3_H
#define MP3_H

    #include "stm32f4xx_hal.h"
    #include <stdint.h>
    #include "cmsis_os2.h"


    // Estados de la tarjeta SD 
    #define HAY_SD  1   // Hay una tarjeta insertada
    #define NO_SD   0   // No hay tarjeta

          // ********** Bytes de COMANDO que se envía al MP3 (4o byte de la trama) **********
    #define CMD_NEXT_SONG     0x01    //Pasar a la siguiente canción en la carpeta
    #define CMD_PREV_SONG     0x02    //Retroceder a la anterior canción en la carpeta
    #define CMD_PLAY_INDEX    0x03    //Reproducir canción mediante su posición en la carpeta
    #define CMD_VOL_UP        0x04    //Subir en 1 el volumen
    #define CMD_VOL_DOWN      0x05    //Bajar en 1 el volumen
    #define CMD_CONF_VOL      0x06    //Poner el volumen al valor deseado
    #define CMD_SEL_DEV       0x09    //Elegir localización de la memoria (elegir dispositivo)
    #define CMD_RESET         0x0C    //Reset del chip
    #define CMD_PLAY          0x0D    //Continuar reproduciendo la canción actual (pausada previamente)
    #define CMD_PAUSE         0x0E    //Pausar la canción actual
    #define CMD_PLAY_FOLDER   0x0F    //Reproducir canción por su ruta (nº carpeta y nºo canción en esa carpeta)
    #define CMD_STOP          0x16    //Parar la reproducción
    #define CMD_SLEEP         0x0A    //Pone a dormir al MP3 (se usa en el modo reposo)
    #define CMD_WAKE          0x0B    //Despierta al MP3

    // **********         Bytes de COMANDO que se enían al MP3 para consultar info         **********
    // **********     (Entrega la info en el 7o Byte, data2 del struct ComandoCompletoMP3) **********
    // **********                (y se consulta mediante la callback del UART)             **********
    #define CMD_INFO_STATUS       0x42    //Informa de el estado del MP3 (0 si STOP, 1 si PLAYING y 2 si PAUSE
    #define CMD_INFO_VOL          0x43    //Informa del volumen actual del MP3
    #define CMD_INFO_ARCHIVOS     0x48    //Manda cuántas canciones hay en la sd (es en total, no por carpeta)
    #define CMD_INFO_CARPETAS     0x4F    //Manda cuántas carpetas hay en la sd

    // Estructura de los 4 bytes del comando del MP3 que vamos a enviar 
    typedef struct {
        uint8_t cmd;      // Comando (Byte 4)
        uint8_t feedback; // Feedback (Byte 5, siempre a 0x00)
        uint8_t data1;    // MSB de la info extra (Byte 6)
        uint8_t data2;    // LSB de la info extra (Byte 7)
        uint8_t hora;     // Hora necesaria para el com-pc
        uint8_t minutos;  // Minutos necesarios para el com-pc
        uint8_t segundos; // Segundos necesarios para el com-pc
    } ComandoCompletoMP3;
    
    // Tipos de Eventos
    #define EVT_MP3_FIN_CANCION   0x01
    #define EVT_MP3_SD_INSERTADA  0x02
    #define EVT_MP3_SD_QUITADA    0x03
    #define EVT_MP3_N_CANCIONES   0x04  // Respuesta a consulta de canciones
    #define EVT_MP3_N_CARPETAS    0x05 // Respuesta a consulta de carpetas
    
    // Estructura para mandar info al principal
    typedef struct {
        uint8_t tipo_evento;  // Ejemplo: EVT_MP3_SONG_END
        uint16_t dato;        // Info númerica (nº canciones o nº carpetas)
    } EventoMP3;

    
    int Init_MP3(void);          // Función de inicialización del hilo MP3
    int Init_Test_MP3(void);     // Función de inicialización del hilo de prueba MP3
    void ThMP3(void *argument);  // Función del hilo principal

#endif // MP3_H
