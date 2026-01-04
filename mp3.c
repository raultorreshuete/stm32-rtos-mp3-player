/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión del Reproductor MP3
  */

#include "mp3.h"
#include "stm32f429xx.h"
#include "Driver_USART.h"

#define MSGQUEUE_OBJECTS 16
#define TRAMA_TRANSMITIDA 0x01 // Flag para sincronizar envío

// ***** Bytes siempre constantes de cada trama al MP3 *****
static const uint8_t MP3_START = 0x7E;    // Byte inicial ($S)
static const uint8_t MP3_VER   = 0xFF;    // Byte de versión (VER)
static const uint8_t MP3_LEN   = 0x06;    // Byte de tamaño (Len)
static const uint8_t MP3_END   = 0xEF;    // Byte final ($O)

// ***** Bytes de Respuesta del del MP3 *****
// Definidos según documentación y hoja de nuevos comandos
#define RESP_SD_INSERTADA     0x3A    // Tarjeta insertada
#define RESP_SD_QUITADA       0x3B    // Tarjeta extraída
#define RESP_FIN_CANCION      0x3D    // Canción terminada
#define RESP_INFO_STATUS      0x42    // Respuesta a consulta de estado (Play/Stop/Pause)
#define RESP_INFO_VOL         0x43    // Respuesta a consulta de volumen
#define RESP_INFO_ARCHIVOS    0x48    // Respuesta a consulta de nº ficheros
#define RESP_INFO_CARPETAS    0x4F    // Respuesta a consulta de nº carpetas

osThreadId_t tid_MP3;       // Id del hilo principal
osThreadId_t tid_ThTestMP3;   // Id del hilo de test

osMessageQueueId_t IdqueueMP3;        // Id de la cola
ComandoCompletoMP3 mp3_cmd_recv;      // Estructura para guardar el comando recibido

osMessageQueueId_t IdColaEventosMP3;  // Id de la cola
EventoMP3 mp3_event;      // Estructura para guardar el comando recibido

// *** (Retocadas) Ahora variables locales de depuración ***
uint8_t estado_SD = HAY_SD;      // Asumimos presente al inicio
uint8_t flag_fin_cancion = 0;    // 1 cuando acaba una canción
uint8_t mp3_status = 0;          // 0 = Stop, 1 = Play, 2 = Pause (Respuesta al 0x42)
uint8_t mp3_volumen_actual = 0;  // Valor de volumen (entre 0 y 30)
uint16_t mp3_num_canciones = 0;  // Total canciones en la SD
uint16_t mp3_num_carpetas = 0;   // Total carpetas en la SD

// DRIVER USART6
extern ARM_DRIVER_USART Driver_USART6; 
static ARM_DRIVER_USART * USARTdrv = &Driver_USART6;

static uint8_t tramaEnvio[8];   // Buffer de envío
static uint8_t byteRX;          // Buffer de recepción (1 byte)

// Variables para recepción de tramas completas (Respuestas)
static uint8_t buffer_rx[10]; 
static uint8_t i = 0;           //Indice para recorrer el buffer_rx

// Prototipos locales
void Test_MP3(void *argument);
void myMP3_Callback(uint32_t event);
static int8_t Init_USART_MP3(void);

// Función inicialización del hilo del MP3
int Init_MP3(void) {
    
    // Inicializar UART a 9600 baudios
    if (Init_USART_MP3() != 0) return -1;

    // Crear la cola de envío al MP3
    IdqueueMP3 = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(ComandoCompletoMP3), NULL);
    if (IdqueueMP3 == NULL) return -1;
  
    // Crear la cola de recepción del MP3
    IdColaEventosMP3 = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(EventoMP3), NULL);
    if (IdColaEventosMP3 == NULL) return -1;

    // Crear el hilo principal
    tid_MP3 = osThreadNew(ThMP3, NULL, NULL);
    if (tid_MP3 == NULL) return -1;
    
    // Activar Recepción para detectar eventos y respuestas
    USARTdrv->Receive(&byteRX, 1);

    return 0;
}

// Función del hilo del MP3
void ThMP3(void *argument) {
    
    while(1) {
        // Esperamos a que nos info del comando a enviar al MP3
        osMessageQueueGet(IdqueueMP3, &mp3_cmd_recv, NULL, osWaitForever);
        
        // Montamos la trama completa que vamos a enviar a enviar
        tramaEnvio[0] = MP3_START;              // 1er Byte fijo
        tramaEnvio[1] = MP3_VER;                // 2o Byte fijo
        tramaEnvio[2] = MP3_LEN;                // 3er Byte fijo
        tramaEnvio[3] = mp3_cmd_recv.cmd;       // DATO IMPORTANTE (5o Byte), lleva el comando
        tramaEnvio[4] = mp3_cmd_recv.feedback;  // DATO IMPORTANTE (6o Byte)
        tramaEnvio[5] = mp3_cmd_recv.data1;     // DATO IMPORTANTE (7o Byte), lleva info adicional si la requiere el comando
        tramaEnvio[6] = mp3_cmd_recv.data2;     // DATO IMPORTANTE (8o Byte), lleva info adicional si la requiere el comando
        tramaEnvio[7] = MP3_END;                // 8o Byte fijo
        
        // Enviamos la trama
        USARTdrv->Send(tramaEnvio, 8);
        
        // Esperamos a la confirmación del envío
        osThreadFlagsWait(TRAMA_TRANSMITIDA, osFlagsWaitAll, osWaitForever);
        
        osDelay(100); // Delay de seguridad (ANTES A 20, FUNCIONAL, PROBAMOS A 100)
    }
}

// Función de inicialización del USART6
static int8_t Init_USART_MP3(void) {
    // Inicializamos el USART driver, con su callback
    if (USARTdrv->Initialize(myMP3_Callback) != ARM_DRIVER_OK ||
        USARTdrv->PowerControl(ARM_POWER_FULL) != ARM_DRIVER_OK ||
    
       //Configuramos el USART a 9600 Baudios (exigido por el fabricante)
        USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                          ARM_USART_DATA_BITS_8 |
                          ARM_USART_PARITY_NONE |
                          ARM_USART_STOP_BITS_1 |
                          ARM_USART_FLOW_CONTROL_NONE, 
                          9600) != ARM_DRIVER_OK ||
    
        // Habilitamos TX y RX
        USARTdrv->Control(ARM_USART_CONTROL_TX, 1) != ARM_DRIVER_OK ||
        USARTdrv->Control(ARM_USART_CONTROL_RX, 1) != ARM_DRIVER_OK) 
    {
        return -1;
    }
    return 0;
}

// Función Callback del USART
  // Importante para procesar el correcto envío y las lecturas de información adicional
void myMP3_Callback(uint32_t event) {
    
    // Confirmación de correcto envío de la trama
    if (event & ARM_USART_EVENT_SEND_COMPLETE) {
        osThreadFlagsSet(tid_MP3, TRAMA_TRANSMITIDA);
    }
    
    // Recepción de tramas enviadas por parte del MP3
    if (event & ARM_USART_EVENT_RECEIVE_COMPLETE) {
        
        // Guardamos byte a byte en el buffer temporal de recepción
        buffer_rx[i] = byteRX;

        // Sincronizar inicio de trama (0x7E)
        if (i == 0 && byteRX != 0x7E) {
            USARTdrv->Receive(&byteRX, 1);
            return;
        }
        i++;

        // Procesar trama (Procesamos 10 bytes en vez de 8 porque el MP3 nos envía 2 extra de checksum [Bytes 8 y 9])
        if (i >= 10) { 
            if (buffer_rx[9] == 0xEF) { // Byte de fin de trama
                
                uint8_t cmd_rx = buffer_rx[3];
                uint8_t data_MSB = buffer_rx[5];
                uint8_t data_LSB = buffer_rx[6];

                switch (cmd_rx) {
                    // *** EVENTOS ASÍNCRONOS ***
                    case RESP_SD_INSERTADA:
                        estado_SD = HAY_SD;   //Indicamos la presencia de la sd
                        
                        mp3_event.tipo_evento = EVT_MP3_SD_INSERTADA;
                        mp3_event.dato = 0;
                        osMessageQueuePut(IdColaEventosMP3, &mp3_event, 0, 0); // Timeout 0 es OBLIGATORIO en callbacks
                    break;
                    
                    case RESP_SD_QUITADA:
                        estado_SD = NO_SD;    //Indicamos la NO presencia de la sd
                        
                        mp3_event.tipo_evento = EVT_MP3_SD_QUITADA;
                        mp3_event.dato = 0;
                        osMessageQueuePut(IdColaEventosMP3, &mp3_event, 0, 0);
                    break;
                    
                    case RESP_FIN_CANCION:
                        flag_fin_cancion = 1;   //Activamos a 1 para indicar que ha finalizado la canción
                        
                        mp3_event.tipo_evento = EVT_MP3_FIN_CANCION;
                        mp3_event.dato = 0;
                        osMessageQueuePut(IdColaEventosMP3, &mp3_event, 0, 0);
                    break;

//                    // *** RESPUESTAS A CONSULTAS DE INOFRMACIÓN DEL SISTEMA ***
//                    case RESP_INFO_STATUS:     // 0x42
//                        mp3_status = data_LSB;   // 0:Stop, 1:Play, 2:Pause
//                    break;
//                    
//                    case RESP_INFO_VOL:        // Guardamos el valor del volumen (entre 0 y 30)
//                        mp3_volumen_actual = data_LSB;
//                    break;
                    
                    case RESP_INFO_ARCHIVOS:   // Guardamos el número de archivos TOTALES
                        mp3_num_canciones = (uint16_t)(data_LSB + (data_MSB << 8));
                        
                        mp3_event.tipo_evento = EVT_MP3_N_CANCIONES;
                        mp3_event.dato = mp3_num_canciones;
                        osMessageQueuePut(IdColaEventosMP3, &mp3_event, 0, 0);
                    break;
                    
                    case RESP_INFO_CARPETAS:   // Guardamos el número de carpetas
                        mp3_num_carpetas = (uint16_t)(data_LSB + (data_MSB << 8));
                    
                        mp3_event.tipo_evento = EVT_MP3_N_CARPETAS;
                        mp3_event.dato = mp3_num_carpetas;
                        osMessageQueuePut(IdColaEventosMP3, &mp3_event, 0, 0);
                    break;
                }
                i = 0; // Trama procesada, reiniciamos índice para la próxima
            } else {
                i = 0; // Error de trama, reiniciamos índice para la próxima
            }
        }
        
        // Reactivamos recepción
        USARTdrv->Receive(&byteRX, 1);
    }
}

/*********************************************************************/
/************ HILO PARA PROBAR EL FUNCIONAMIENTO COMPLETO ************/
/*********************************************************************/

// Función de inicialización del test
int Init_Test_MP3(void) {
    tid_ThTestMP3 = osThreadNew(Test_MP3, NULL, NULL);
    if (tid_ThTestMP3 == NULL) return -1;
    return 0;
}

// Función del hilo del test
void Test_MP3(void *argument) {
    ComandoCompletoMP3 tramaPrueba;
    uint8_t fase = 0;               // Gestiona cambio de estado
    uint32_t tiempo_espera = 2000;  // Tiempo por defecto

    while (1) {
        
        // Limpiamos parámetros de la cola
        tramaPrueba.feedback = 0x00;
        tramaPrueba.data1 = 0x00;
        tramaPrueba.data2 = 0x00;
        tiempo_espera = 2000; // Reset a espera corta

        switch(fase) {
            case 0:
                // SELECT DEVICE
                tramaPrueba.cmd = CMD_SEL_DEV;
                tramaPrueba.data2 = 0x02; 
                tiempo_espera = 1000;
                break;

            case 1:
                // CONSULTAR NÚMERO DE CARPETAS (ver en los watches la variable mp3_num_carpetas)
                tramaPrueba.cmd = CMD_INFO_CARPETAS;
                tiempo_espera = 1000;
                break;
                
            case 2:
                // CONFIGURAR VOLUMEN 
                tramaPrueba.cmd = CMD_CONF_VOL;
                tramaPrueba.data2 = 0x14; // Volumen 20
                tiempo_espera = 1000;
                break;

            case 3:
                // CONSULTAR VOLUMEN
                // Verificar que se puso a 20  (ver en los watches la variable mp3_volumen_actual)
                tramaPrueba.cmd = CMD_INFO_VOL;
                tiempo_espera = 1000;
                break;
                
            case 4:
                // PLAY FOLDER 02 / CANCIÓN 003 
                tramaPrueba.cmd = CMD_PLAY_FOLDER;
                tramaPrueba.data1 = 0x02; 
                tramaPrueba.data2 = 0x03; 
                tiempo_espera = 30000; // Escuchar 30 seg
                break;

            case 5:
                // CONSULTAR NÚMERO DE ARCHIVOS  (ver en los watches la variable mp3_num_canciones)
                // (es número total de archivos contando los de cada carpeta, no archivos en la carpeta)
                tramaPrueba.cmd = CMD_INFO_ARCHIVOS;
                tiempo_espera = 1000; 
                break;
                
            case 6:
                // PAUSE 
                tramaPrueba.cmd = CMD_PAUSE;
                tiempo_espera = 3000; // Silencio 3 seg
                break;

            case 7:
                // CONSULTAR ESTADO
                // Debería responder que está en PAUSA (0x02)
                tramaPrueba.cmd = CMD_INFO_STATUS;
                tiempo_espera = 1000;
                break;

            case 8:
                // PLAY/RESUME 
                tramaPrueba.cmd = CMD_PLAY;
                tiempo_espera = 5000; // Escuchar 5 seg más
                break;
                
            case 9:
                // NEXT SONG 
                tramaPrueba.cmd = CMD_NEXT_SONG;
                tiempo_espera = 30000; 
                break;
                
            case 10:
                // PREV SONG 
                tramaPrueba.cmd = CMD_PREV_SONG;
                tiempo_espera = 3000; 
                break;

            case 11:
                // SLEEP (necesario para especificación de MODO REPOSO)
                tramaPrueba.cmd = CMD_SLEEP;
                tiempo_espera = 3000;
                break;

            case 12:
                // DESPERTAR (necesario al volver al MODO REPRODUCCIÓN)
                tramaPrueba.cmd = CMD_WAKE;
                tiempo_espera = 1000; 
                break;
                
            case 13:
                // RESET 
                tramaPrueba.cmd = CMD_RESET;
                tiempo_espera = 2000;
                break;
        }

        // Enviamos a la cola
        osMessageQueuePut(IdqueueMP3, &tramaPrueba, 0, 0);
        
        // Avanzamos fase
        fase++;
        if(fase > 13) fase = 0; // Reiniciar ciclo tras el último comando

        osDelay(tiempo_espera); // Espera personalizada para cada prueba
    }
}