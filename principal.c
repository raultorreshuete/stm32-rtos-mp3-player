/**
SBM - PROYECTO FINAL
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
*/

#include "cmsis_os2.h"                  // CMSIS RTOS header file
#include "principal.h"
#include "clock.h"
#include "joystick.h"
#include "lcd.h"
#include "com.h"
#include "pwm.h"
#include "temp.h"
#include "mp3.h"
#include "pot.h"
#include "rgb.h"
#include <stdio.h>
#include <string.h> 

// *** COLAS EXTERNAS ***
extern osMessageQueueId_t IdcolaJoystick;		// Cola del Joystick
extern osMessageQueueId_t IdcolaLCD;				// Cola del LCD
extern osMessageQueueId_t IdqueueComTrans;	// Cola del COM-PC
extern osMessageQueueId_t IdcolaPWM;				// Cola del PWM
extern osMessageQueueId_t Idcolatemp;				// Cola del sensor de temperatura
extern osMessageQueueId_t IdqueueMP3;      	// Cola de los comandos del MP3 (mandamos)
extern osMessageQueueId_t IdColaPot;      	// Cola del Potenciómetro
extern osMessageQueueId_t IdColaRGB;       	// Cola de los LEDs RGB
extern osMessageQueueId_t IdqueueMP3;      	// Cola de los comandos del MP3 (mandamos)
extern osMessageQueueId_t IdColaEventosMP3;	// Cola de los eventos del MP3 (recibimos)

// *** VARIABLES GLOBALES/EXTERNAS ***
extern infoJoys datosJoys;
extern infoLCD datosLCD;
extern HoraCompleta hora;

// *** VARIABLES LOCALES ***
osThreadId_t tid_principal;					
static uint8_t estado = 1;         		// 1: REPOSO, 2: REPRODUCCIÓN, 3:PROGRAMACIÓN DE HORA
static uint8_t estado_anterior = 0; 	// Para detectar transiciones (Wake/Sleep)
static uint8_t safe = 0;							// Lock para asegurar una única petición de INFO_CARPETAS

// *** VARIABLES PARA GESTIONAR EL ENVÍO DE INFO AL LCD ***
static char texto1[80];											// Buffer de la línea 1
static char texto2[80];											// Buffer de la línea 2
static double temperaturaLeida = 0.0;				// Por defecto a 0
static double temperaturaActualizada = 0.0;	// Por defecto a 0
static uint8_t volumenSistema = 15; 				// Por defecto al valor medio

// *** VARIABLES PARA GESTIONAR LA REPRODUCCIÓN DEL MP3 ***
static uint8_t carpetaActual = 1;			// Indica la carpeta en la que nos encontramos
static uint8_t cancionActual = 1;			// Indica la canción dentro de la carpeta en la que nos encontramos
static uint8_t minutosCancion = 0;		// Maneja los minutos de la canción que se está reproduciendo
static uint8_t segundosCancion = 0;		// Maneja los segundos de la canción que se está reproduciendo
static uint8_t play_notpause = 0; 		// 0 para Pausa, 1 para Play

// *** VARIABLES PARA GESTIONAR LA PROGRAMACIÓN DE HORA ***
static int8_t posicionHora = 0;			// Del 0 al 5, indica la posición (decena horas, unidad horas, decena minutos...)
static int8_t valorHora = 0; 				// 1 = Indica SUMA, -1 indica RESTA
static HoraCompleta horaTemporal;		// Estructura HoraCompleta para guardar la hora editada antes de actualizarla

// *** VARIABLES PARA GESTIONAR INFO DEL MP3 (DATOS RECIBIDOS) ***
static uint16_t num_carpetas_total = 0;	// Guarda la info de cuántas carpetas hay en la SD
static uint8_t hay_tarjeta = 1;					// Guarda el estado de la SD localmente. Si hay tarjeta a 1, si no hay a 0 


// *** VARIABLES PARA OPCIONAL 1 (REPRODUCCIÓN CONTINUA) ***
static uint8_t repro_continua = 0;          // Variable que indiaca si se está en modo de reproducción continua (1) o no (0)
static uint32_t last_time_fin_cancion = 0;  // Ayuda a evitar dobles llegadas de EVT_MP3_FIN_CANCION que generaban saltos  

// Funciones auxiliares privados
static void gestionarCambioEstado(void);
static void seleccionarCarpetaMP3(uint8_t direccion);
static void programar(void);
static void ajustarHora(void);

// Función de inicialización del hilo principal
int Init_principal (void) {
	
		tid_principal = osThreadNew(Thprincipal, NULL, NULL);
		if (tid_principal == NULL) return(-1);
	
		return(0);
}
 
// Función del hilo principal
void Thprincipal (void *argument) {

		// Status para recoger la cola de estos módulos sólo cuando haya interacción
    osStatus_t statusJoys;
    osStatus_t statusTemp;
    osStatus_t statusPot;
		osStatus_t statusMP3;
    
		// Variables locales para enviar o recoger información de las colas
    infoPot datosPot; 							// Estructura local para recibir del POT
    ComandoCompletoMP3 cmdMP3; 			// Estructura local para enviar comandos
    uint32_t last_lcd_update = 0;		// Variable local para el refresco de la pantalla LCD
		EventoMP3 eventoMP3;						// Estructura local para recoger eventos del MP3
    infoRGB colorRGB;               // Estructura local para enviar color (y volumen - OPCIONAL 5) al RGB 
	  uint32_t flags_recibidos; 			// Variable para guardar flags
    
		// ********** INICIALIZACIÓN *********
    estado = 1; 
    estado_anterior = 0; 						// Forzamos la ejecución de la lógica de entrada al estado 1
  
    // Enviamos un RESET preventivo al MP3 para limpiar posibles estados raros al encender la placa
    cmdMP3.cmd = CMD_RESET; 
    cmdMP3.feedback = 0;
		cmdMP3.data1 = 0;
		cmdMP3.data2 = 0;
    // Envio al MP3
    osMessageQueuePut(IdqueueMP3, &cmdMP3, 0, 0);
    // Envio al COM-PC (TeraTerm) + la hora del clock.c
    cmdMP3.hora = hora.horas;
    cmdMP3.minutos = hora.minutos;
    cmdMP3.segundos = hora.segundos;
    osMessageQueuePut(IdqueueComTrans, &cmdMP3, 0, 0);
    osDelay(500); // Tiempo para que el chip se reinicie

    // Enviamos comando SELECT DEVICE para elegir la SD
    cmdMP3.cmd = CMD_SEL_DEV; 
    cmdMP3.data2 = 0x02; 
    // Envio al MP3
    osMessageQueuePut(IdqueueMP3, &cmdMP3, 0, 0);
    // Envio al COM-PC (TeraTerm) + la hora del clock.c
    cmdMP3.hora = hora.horas;
    cmdMP3.minutos = hora.minutos;
    cmdMP3.segundos = hora.segundos;
    osMessageQueuePut(IdqueueComTrans, &cmdMP3, 0, 0);
    osDelay(200); 

    // Enviamos comando PLAY para que comience sonando (en cuanto entre al MODO REPRODUCCIÓN)
    cmdMP3.cmd = CMD_PLAY; 
    // Envio al MP3
    osMessageQueuePut(IdqueueMP3, &cmdMP3, 0, 0);
    // Envio al COM-PC (TeraTerm) + la hora del clock.c
    cmdMP3.hora = hora.horas;
    cmdMP3.minutos = hora.minutos;
    cmdMP3.segundos = hora.segundos;
    osMessageQueuePut(IdqueueComTrans, &cmdMP3, 0, 0);
		osDelay(200); 
  
  while (1) {
        
        // En cada iteración leemos de las colas del Joystick, temperatura, potenciómetro y MP3
        statusJoys = osMessageQueueGet(IdcolaJoystick, &datosJoys, NULL, 0);
        statusTemp = osMessageQueueGet(Idcolatemp, &temperaturaLeida, NULL, 0);
        statusPot  = osMessageQueueGet(IdColaPot, &datosPot, NULL, 0);
		    statusMP3 = osMessageQueueGet(IdColaEventosMP3, &eventoMP3, NULL, 0);
				
				// Sólo si hay algo en la cola, actualizamos temperatura
				if(statusTemp == osOK){
            temperaturaActualizada = temperaturaLeida;
        }
				
				// Solo si hay algo en la cola, actualizamos volumen
        if(statusPot == osOK){
            volumenSistema = datosPot.nivel_volumen;
					
            // Si estamos en MODO REPRODUCCIÓN, actualizamos volumen al vuelo
            if(estado == 2){
                cmdMP3.cmd = CMD_SET_VOL;
                cmdMP3.data2 = volumenSistema;
                // Envio al MP3
                osMessageQueuePut(IdqueueMP3, &cmdMP3, 0, 0);
                // Envio al COM-PC (TeraTerm) + la hora del clock.c
                cmdMP3.hora = hora.horas;
                cmdMP3.minutos = hora.minutos;
                cmdMP3.segundos = hora.segundos;
                osMessageQueuePut(IdqueueComTrans, &cmdMP3, 0, 0);
              
                // (OPCIONAL 5) Mandamos información del volumen al RGB para que modifique su brillo
                colorRGB.estado = RGB_PLAY;
                colorRGB.volumen = volumenSistema;
                osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
            }
        }
     
        // Fuera de la máquina de estados, solo si hay algo en la cola,
					// leemos la información que nos haya podido enviar el MP3
        if (statusMP3 == osOK) {
            switch(eventoMP3.tipo_evento) {
								 
                case EVT_MP3_FIN_CANCION:		// Si evento de "fin de canción"
                  
                    // Este if sólo permite la entrada de un evento de fin de canción 
                      // cuando han pasado más de 2 segundos desde el último,
                      // evitando bugs de dobles llegadas y saltos de canciones
                    if (HAL_GetTick() - last_time_fin_cancion > 2000) { 
                      
                        // Actualizamos la marca de tiempo
                        last_time_fin_cancion = HAL_GetTick();
                      
                        // Indicamos que entramos en PAUSA y encendemos del color corresponidente (AZUL) el RGB
                        play_notpause = 0;			
                        colorRGB.estado = RGB_PAUSE;
                        osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
                        
                        // Si se ha activado la reproducción continua (OPCIONAL 1)
                        if(repro_continua == 1){
                            
                            // Pasamos a la siguiente canción y reseteamos tiempo de canción
                            cmdMP3.cmd = CMD_NEXT_SONG;
                            cancionActual++;
                            segundosCancion=0;
                            minutosCancion=0;
                          
                            // Envio al MP3
                            osMessageQueuePut(IdqueueMP3, &cmdMP3, 0, 0);
                            // Envio al COM-PC (TeraTerm) + la hora del clock.c
                            cmdMP3.hora = hora.horas;
                            cmdMP3.minutos = hora.minutos;
                            cmdMP3.segundos = hora.segundos;
                            osMessageQueuePut(IdqueueComTrans, &cmdMP3, 0, 0);
                          
                            // Indicamos que estamos en PLAY, y notificamos
                              // Notificamos con el encendido del led VERDE (+ info del volumen para OPCIONAL 5)
                            play_notpause = 1;			
                            colorRGB.estado = RGB_PLAY;
                            colorRGB.volumen = volumenSistema;
                            osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
                      
                        }
                    }
                break;

                case EVT_MP3_SD_QUITADA:		// Si evento de "se ha extraído la tarjeta"
										// Indicamos que NO hay tarjeta
                    hay_tarjeta = 0;
                break;

                case EVT_MP3_SD_INSERTADA:	// Si evento de "se ha introducido la tarjeta"
										// Indicamos que la SI hay tarjeta
                    hay_tarjeta = 1;
                break;
                    
                case EVT_MP3_N_CARPETAS:		// Si evento de "se ha preguntado por el número de carpetas"
                    // Guardamos el número de carpetas
										num_carpetas_total = eventoMP3.dato - 1; // El menos 1 es por la carpeta fantasma de SystemVolumeInformation que no hemos conseguido eliminar
                break;
            }
        }
				// Gestionamos la transición entre estados (Se ejecuta solo 1 vez al cambiar de estado)
        if (estado != estado_anterior) {
            gestionarCambioEstado();
            estado_anterior = estado;
					
            // Forzamos actualización inmediata de LCD
            last_lcd_update = 0; 
        }

        // *** MÁQUINA DE ESTADOS ***
        switch(estado){
            
            // **************************************
            // *****		ESTADO 1: MODO REPOSO		*****
            // **************************************
            case 1: 
                // A. JOYSTICK
                if(statusJoys == osOK){
									
                    // Sólo atendemos pulsación larga del botón central para salir
                    if(datosJoys.tipo_pulsacion == 1 && datosJoys.GPIO_PIN == GPIO_PIN_15){
											
												// Si hay tarjeta, se permite entrar en el MODO REPRODUCCIÓN
                        if (hay_tarjeta == 1) {
                            estado = 2;		// REPRODUCCIÓN
													
												// Si no hay tarjeta, seguimos en MODO REPOSO y notificamos
                        } else {
														// Notificamos con el encendido del RGB en ROJO
                            colorRGB.estado = RGB_ERROR;
                            osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
													
                            // Notificamos con 3 pitidos estridentes del buzzer
                            uint8_t bip = 1; 
                            osMessageQueuePut(IdcolaPWM, &bip, 0, 0);
                        }
                    }
                }

                // B. LCD 
								// Si (leyendo y comparando con el tick del sistema), ha pasado más de 500ms
									// desde quese ha actualizado el LCD, se actualiza.
                if (HAL_GetTick() - last_lcd_update > 500) {
                    
                    // Aseguramos que no aparezca la flechita fuera de PROGRAMACIÓN DE HORA
                    datosLCD.posCursor = -1;
									
										// Metemos el formato de las 2 líneas del LCD con los valores en strings
                    sprintf(texto1, "SBM 2025  Ta %02.1f*C", temperaturaLeida);               
                    sprintf(texto2, "      %02d:%02d:%02d", hora.horas, hora.minutos, hora.segundos);
									
										// Copiamos esos strings a su contra parte en el struct a enviar por la cola y enviamos al LCD
                    strcpy(datosLCD.frase1, texto1);
                    strcpy(datosLCD.frase2, texto2);
                    osMessageQueuePut(IdcolaLCD, &datosLCD, 0, 0);
									
										// Actualizamos el valor de tiempo del tick del sistema en el que hemos refrescado el LCD
                    last_lcd_update = HAL_GetTick();
                }
                break;
            
            // ********************************************
            // *****		ESTADO 2: MODO REPRODUCCIÓN		*****
            // ********************************************
            case 2:
                // A. COMPROBACIÓN SD (Prioridad máxima)
								// Si se extrae la tarjeta, mandamos de vuelta al reposo y notificamos
                if (hay_tarjeta == 0) {
										// Notificación RGB (ROJO)
                    colorRGB.estado = RGB_ERROR;
                    osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
                    
                    // Notificación BUZZER 
                    uint8_t bip = 1; 
                    osMessageQueuePut(IdcolaPWM, &bip, 0, 0);
										
										// Reseteamos el contador de la canción para cuando volvamos a entrar
                    minutosCancion = 0; 
										segundosCancion = 0;
										
										// Abrimos seguro de info carpetas 
                    safe = 0;
                    
										// Volvemos a REPOSO
                    estado = 1; 
                    break; 
                  } 
								// Si hay tarjeta y el lock está abierto
                else if (hay_tarjeta == 1 && safe == 0){
                      osDelay(500); //osDelay necesario para que lea bien todas las carpetas
											// Mandamos petición de NÚMERO DE CARPETAS y limpiamos posibles residuos de data y demás
                      cmdMP3.cmd = CMD_INFO_CARPETAS; 
                      cmdMP3.feedback = 0x00;
                      cmdMP3.data1 = 0x00;
                      cmdMP3.data2 = 0x00;
                      // Envio al MP3
                      osMessageQueuePut(IdqueueMP3, &cmdMP3, 0, 0);
                      // Envio al COM-PC (TeraTerm) + la hora del clock.c
                      cmdMP3.hora = hora.horas;
                      cmdMP3.minutos = hora.minutos;
                      cmdMP3.segundos = hora.segundos;
                      osMessageQueuePut(IdqueueComTrans, &cmdMP3, 0, 0);
                      osDelay(500);
									
											// Cerramos el lock para no volver a preguntar nunca más
												// por las carpetas a la SD hasta que sea extraída 
                      safe = 1;
                  }
								
                // B. CONTADOR TIEMPO DE CANCIÓN
                // Comprobamos si ha llegado el aviso de segundo nuevo del clock.c (timeout a 0 para no bloquear)
                flags_recibidos = osThreadFlagsWait(FLAG_SEGUNDO_NUEVO, osFlagsWaitAny, 0);
                
                // (Los errores en CMSIS-RTOS2 tienen el bit 31 a 1, son números muy grandes)
                if ((flags_recibidos & 0x80000000U) == 0) {
                  
                    // Si flags_recibidos tiene el bit activo, es que ha pasado 1 segundo
                    if ((flags_recibidos & FLAG_SEGUNDO_NUEVO) == FLAG_SEGUNDO_NUEVO) {
                        
                        // Lógica de contador de canción:
													// Sólo sumamos si estamos en REPRODUCCION y no estamos en PAUSA
                        if (estado == 2 && play_notpause == 1) {
                            segundosCancion++;
                            if(segundosCancion > 59){
                                segundosCancion = 0;
                                minutosCancion++;
                            }
                            // Forzamos actualización del LCD para ver el cambio
                            last_lcd_update = 0; 
                        }
                    }
                }

                // C. JOYSTICK
                if (statusJoys == osOK){
                    // Sólo atendemos pulsación larga del botón central para salir
                    if(datosJoys.tipo_pulsacion == 1 && datosJoys.GPIO_PIN == GPIO_PIN_15){
                        estado = 3;				// PROGRAMACIÓN DE HORA
												// Reseteamos el tiempo de la canción
                        minutosCancion = 0;
												segundosCancion = 0;
                        datosJoys.tipo_pulsacion = 0;
                    }
                    // Si nos llega pulsación larga del botón derecho, activamos reproducción continua y notificamos
                    else if(datosJoys.tipo_pulsacion == 1 && datosJoys.GPIO_PIN == GPIO_PIN_11){
                        repro_continua = 1;
                        // Notificamos con doble bip
                        uint8_t bip = 0;
                        osMessageQueuePut(IdcolaPWM, &bip, 0, 0);
                        osDelay(200);
                        osMessageQueuePut(IdcolaPWM, &bip, 0, 0);
                    }
                    // Si nos llega pulsación larga del botón izquierdo, desactivamos reproducción continua y notificamos
                    else if(datosJoys.tipo_pulsacion == 1 && datosJoys.GPIO_PIN == GPIO_PIN_14){
                        repro_continua = 0;
                        // Notificamos con doble bip
                        uint8_t bip = 0;
                        osMessageQueuePut(IdcolaPWM, &bip, 0, 0);
                        osDelay(200);
                        osMessageQueuePut(IdcolaPWM, &bip, 0, 0);
                    }
                    // Control del MP3 (En las pulsaciones cortas)
                    else if(datosJoys.tipo_pulsacion == 0){
                        // Reset de precaución de los valores del comando MP3 que se enviarán
                        cmdMP3.cmd = 0;
												cmdMP3.feedback = 0;
												cmdMP3.data1 = 0;
												cmdMP3.data2 = 0;
                        
                        // Bip de cada pulsación
                        uint8_t bip = 0;
                        osMessageQueuePut(IdcolaPWM, &bip, 0, 0);

                        switch(datosJoys.GPIO_PIN){
                            case GPIO_PIN_11: // RIGHT
																// Pasamos a la siguiente canción y reseteamos tiempo de canción
                                cmdMP3.cmd = CMD_NEXT_SONG;
                                cancionActual++;
																segundosCancion=0;
																minutosCancion=0;
                            
                                // Forzamos estado PLAY visual y lógico
                                  // + mandamos info de volumen al RGB (OPCIONAL 5)
                                play_notpause = 1; 
                                colorRGB.estado = RGB_PLAY;
                                colorRGB.volumen = volumenSistema;
                                osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
                            break;
														
                            case GPIO_PIN_14: // LEFT
																// Volvemos a la anterior canción y reseteamos tiempo de canción
                                cmdMP3.cmd = CMD_PREV_SONG;
														
																// Si es la primera canción, no nos movemos
                                if(cancionActual>1) cancionActual--; 
                                segundosCancion=0;
																minutosCancion=0;
                            
                                // Forzamos estado PLAY visual y lógico
                                  // + mandamos info de volumen al RGB (OPCIONAL 5)
                                play_notpause = 1; 
                                colorRGB.estado = RGB_PLAY;
                                colorRGB.volumen = volumenSistema;
                                osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
                            break;
														
                            case GPIO_PIN_10: // UP
																// Lamamos a la función para pasar a la siguiente carpeta
                                seleccionarCarpetaMP3(0); 
														
                                // Forzamos estado PLAY visual y lógico
                                  // + mandamos info de volumen al RGB (OPCIONAL 5)
                                play_notpause = 1; 
                                colorRGB.estado = RGB_PLAY;
                                colorRGB.volumen = volumenSistema;
                                osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
                            break;
														
                            case GPIO_PIN_12: // DOWN
																// Lamamos a la función para retroceder a la anterior carpeta
                                seleccionarCarpetaMP3(1);
                            
                                // Forzamos estado PLAY visual y lógico
                                  // + mandamos info de volumen al RGB (OPCIONAL 5)
                                play_notpause = 1; 
                                colorRGB.estado = RGB_PLAY;
                                colorRGB.volumen = volumenSistema;
                                osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
                            break;
														
                            case GPIO_PIN_15: // CENTER
																// Si estamos en play
                                if(play_notpause == 1){
																		// Comando de PAUSA y notificación correspondiente del LED (AZUL) 
                                      // + mandamos info de volumen al RGB (OPCIONAL 5)
                                    cmdMP3.cmd = CMD_PAUSE;
                                    play_notpause = 0;
                                    colorRGB.estado = RGB_PAUSE;
                                    colorRGB.volumen = volumenSistema;
                                    osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
                                } else {
																		// Comando de PLAY y notificación correspondiente del LED (VERDE)
                                      // + mandamos info de volumen al RGB (OPCIONAL 5)
                                    cmdMP3.cmd = CMD_PLAY;
                                    play_notpause = 1;
                                    colorRGB.estado = RGB_PLAY;
                                    colorRGB.volumen = volumenSistema;
                                    osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
                                }
                            break;
                        }
                        // Confirmamos el envío por la cola del comando elegido en el switch anterior
                        if(cmdMP3.cmd != 0){
                            // Envio al MP3
                            osMessageQueuePut(IdqueueMP3, &cmdMP3, 0, 0);
                            // Envio al COM-PC (TeraTerm) + la hora del clock.c
                            cmdMP3.hora = hora.horas;
                            cmdMP3.minutos = hora.minutos;
                            cmdMP3.segundos = hora.segundos;
                            osMessageQueuePut(IdqueueComTrans, &cmdMP3, 0, 0);
                        }
                    }
                }
            
                // D. LCD
								// Mismo procedimiento que en REPOSO y PROGRAMACIÓN
                if (HAL_GetTick() - last_lcd_update > 500) {
                    datosLCD.posCursor = -1;
                    sprintf(texto1, " F:%02d C:%02d   Vol: %02d", carpetaActual, cancionActual, volumenSistema);
                    // Comprobamos si estamos en reproducción continua
                    if(repro_continua == 1){
                        // Si lo estamos, añadimos marca de CC
                        sprintf(texto2, "      T: %02d:%02d   CC", minutosCancion, segundosCancion);
                    } else {
                        sprintf(texto2, "      T: %02d:%02d", minutosCancion, segundosCancion);
                    }
                    strcpy(datosLCD.frase1, texto1);
                    strcpy(datosLCD.frase2, texto2);
                    osMessageQueuePut(IdcolaLCD, &datosLCD, 0, 0);
                    last_lcd_update = HAL_GetTick();
                }
                break;
            
            // ****************************************************
            // *****		ESTADO 3: MODO PROGRAMACIÓN DE HORA		*****
            // ****************************************************
            case 3:
                // A. JOYSTICK
                if(statusJoys == osOK){
										// Sólo atendemos pulsación larga del botón central para salir
                    if(datosJoys.tipo_pulsacion == 1 && datosJoys.GPIO_PIN == GPIO_PIN_15){
                        estado = 1;							// REPOSO
                        datosJoys.tipo_pulsacion = 0;
                        posicionHora = 0; 			// Reseteamos el cursor
                    }
                    else {
												// Llamamos a la lógica de programar y ajustar hora
                        programar();
                        last_lcd_update = 0; 		// Refresco inmediato para feedback visual
                    }
                }
                
                // B. LCD
								// Mismo procedimiento que en REPOSO y REPRODUCCIÓN
                if (HAL_GetTick() - last_lcd_update > 500) {
                    datosLCD.posCursor = posicionHora;
                    sprintf(texto1, "  HORA   Ta %02.1f*C", temperaturaLeida);                
                    sprintf(texto2, "      %02d:%02d:%02d", horaTemporal.horas, horaTemporal.minutos, horaTemporal.segundos); 
                    strcpy(datosLCD.frase1, texto1);
                    strcpy(datosLCD.frase2, texto2);
                    osMessageQueuePut(IdcolaLCD, &datosLCD, 0, 0);
                    last_lcd_update = HAL_GetTick();
                }
                break;
        }

        osDelay(50);		// Delay de seguridad para no reventar el hilo
        osThreadYield(); 
  }
}

// *************************************************************************
// ********************			 FUNCIONES AUXILIARES 			********************
// *************************************************************************

// Función auxiliar para asegurar que se cambia de estado UNA ÚNICA VEZ
static void gestionarCambioEstado(void) {
    ComandoCompletoMP3 cmd;
    //estadosRGB color;
    infoRGB colorRGB;
		
    if (estado == 1) { // Entrando a REPOSO
        // Mandamos el MP3 a dormir
        cmd.cmd = CMD_SLEEP; 
				cmd.feedback = 0;
				cmd.data1 = 0; 
				cmd.data2 = 0;
        osMessageQueuePut(IdqueueMP3, &cmd, 0, 0);
        
        // Si hay tarjeta SD, se apaga el LED
        if (hay_tarjeta == 1){
            // Y apagamos el RGB
            colorRGB.estado = RGB_OFF;
            colorRGB.volumen = volumenSistema;
            osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
        } 
    }
    else if (estado == 2) { // Entrando a REPRODUCCIÓN
        // Despertamos al MP3
        cmd.cmd = CMD_WAKE;
        osMessageQueuePut(IdqueueMP3, &cmd, 0, 0);
        osDelay(200); // Le damos tiempo a que se despierte antes de meterle más comandos
      
        // Volvemos a mandarle el comando de seleccionar la tarjeta SD ya que
					// al dormir, el chip a veces se raya y se olvida de esa info
        cmd.cmd = CMD_SEL_DEV;
        cmd.data2 = 0x02;
        osMessageQueuePut(IdqueueMP3, &cmd, 0, 0);
        osDelay(200); // Otro delay para que le de tiempo a montar la SD
        
        // Reseteo inicial del volumen
        cmd.cmd = CMD_SET_VOL; 
				cmd.data2 = volumenSistema;
        osMessageQueuePut(IdqueueMP3, &cmd, 0, 0);
        
        // Retomamos la reproducción con la canción y carpeta últimas 
					// (si es la primera iteración empezará en 01, 001)
        cmd.cmd = CMD_PLAY_FOLDER;
				cmd.data1 = carpetaActual;
				cmd.data2 = cancionActual;
        osMessageQueuePut(IdqueueMP3, &cmd, 0, 0);
        
				// Ponemos a 1 nuestra variable que indica el estado de la canción (PLAY o PAUSA)
        play_notpause = 1;
        
        // Encendemos el LED correspondiente al estado de PLAY (VERDE)
        colorRGB.estado = RGB_PLAY;
        colorRGB.volumen = volumenSistema;
        osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
    }
    else if (estado == 3) { // Entrando a PROGRAMACIÓN
        // Copiamos la hora real a la temporal para editarla sin problema
        horaTemporal.horas = hora.horas;
        horaTemporal.minutos = hora.minutos;
        horaTemporal.segundos = hora.segundos;
      
        // Ordenamos que se pause la música
        cmd.cmd = CMD_PAUSE;
        osMessageQueuePut(IdqueueMP3, &cmd, 0, 0);
			
        // Ponemos a 0 nuestra variable que indica el estado de la canción (PLAY o PAUSA)
        play_notpause = 0;
        
        // Encendemos el LED correspondiente al estado de PAUSA (AZUL)
        colorRGB.estado = RGB_PAUSE;
        colorRGB.volumen = volumenSistema;
        osMessageQueuePut(IdColaRGB, &colorRGB, 0, 0);
    }
}

// Función auxiliar para elegir la carpeta que se está reproducciendo,
	// pasamos como parámetro la dirección (arriba '0' o abajo '1')
	// Seguimos un patrón cíclico (de la última subimos a la primera y de la primera bajamos a la última)
static void seleccionarCarpetaMP3(uint8_t direccion){
    ComandoCompletoMP3 cmd;
    cmd.feedback = 0x00;

    if(direccion == 0){ // Siguiente
        carpetaActual++;
				// Si superamos el límite de carpetas, volvemos a la primera
        if(carpetaActual > num_carpetas_total)
          carpetaActual = 1;
    }
    else if (direccion == 1){ // Anterior
        if (carpetaActual > 1)
          carpetaActual--;
				// Si bajamos de la primera carpeta, pasamos a la última
        else 
          carpetaActual = num_carpetas_total;
    }
		// Al cambiar de carpeta siempre iniciamos con la primera canción
    cancionActual = 1;
		
		// Reseteamos el contador de tiempo al cambiar de canción
    minutosCancion = 0; 
		segundosCancion = 0;
    
		// Mandamos el comando de play al MP3
    cmd.cmd = CMD_PLAY_FOLDER;
    cmd.data1 = carpetaActual;
    cmd.data2 = cancionActual;
    // Envio al MP3
    osMessageQueuePut(IdqueueMP3, &cmd, 0, 0);
    // Envio al COM-PC (TeraTerm) + la hora del clock.c
    cmd.hora = hora.horas;
    cmd.minutos = hora.minutos;
    cmd.segundos = hora.segundos;
    osMessageQueuePut(IdqueueComTrans, &cmd, 0, 0);
}

// Función auxiliar que permite moverse por las decenas y unidades 
  // de los 3 parámetros de la hora, indicar el cambio (suma o resta) y aplicarlo.
static void programar(void){
    // Tramo de navegación entre posiciones
    if(datosJoys.GPIO_PIN == GPIO_PIN_11){ // RIGHT
        posicionHora++;
        if (posicionHora > 5) posicionHora = 0;
    }
    else if(datosJoys.GPIO_PIN == GPIO_PIN_14){ // LEFT
        posicionHora--;
        if (posicionHora < 0) posicionHora = 5;
    }
    // Tramo de suma o resta de valor
    else if(datosJoys.GPIO_PIN == GPIO_PIN_10){ // UP
        valorHora = 1; 
        ajustarHora();
    }
    else if(datosJoys.GPIO_PIN == GPIO_PIN_12){ // DOWN
        valorHora = -1; 
        ajustarHora();
    }
    else if(datosJoys.GPIO_PIN == GPIO_PIN_15){ // CENTER
        // Confirmamos el cambio de hora
        hora.horas = horaTemporal.horas;
        hora.minutos = horaTemporal.minutos;
        hora.segundos = horaTemporal.segundos;
      
        posicionHora = 0;
      
        // Bip de cada pulsación
        uint8_t bip = 0;
        osMessageQueuePut(IdcolaPWM, &bip, 0, 0);
    }
    datosJoys.GPIO_PIN = 0;
}

// Función auxiliar que realiza el ajuste de horas para su programación:
  // Permite moverse entre decenas y unidades de las 3 componentes
static void ajustarHora(void){
    int8_t digito; 
    switch(posicionHora){
        case 0: // Decenas Hora
            // Casteamos a int con signo (para poder hacer más fácilmente la suma o resta) 
              // y pillamos el resto para las unidades
            digito = (int8_t)(horaTemporal.horas / 10);
            digito += valorHora;
            // Configuramos los límites
            if (digito > 2) digito = 0; 
            if (digito < 0) digito = 2;
            // Aplicamos el cambio al reloj principal (el de clock.c)
            horaTemporal.horas = (uint8_t)((digito * 10) + (horaTemporal.horas % 10));
        break;
        
        case 1: // Unidades Hora
            digito = (int8_t)(horaTemporal.horas % 10);
            digito += valorHora;
            if (digito > 9) digito = 0;
            if (digito < 0) digito = 9;
            horaTemporal.horas = (uint8_t)((horaTemporal.horas / 10 * 10) + digito);
        break;
        
        case 2: // Decenas Minuto
            digito = (int8_t)(horaTemporal.minutos / 10);
            digito += valorHora;
            if (digito > 5) digito = 0;
            if (digito < 0) digito = 5;
            horaTemporal.minutos = (uint8_t)((digito * 10) + (horaTemporal.minutos % 10));
        break;
        
        case 3: // Unidades Minuto
            digito = (int8_t)(horaTemporal.minutos % 10);
            digito += valorHora;
            if (digito > 9) digito = 0;
            if (digito < 0) digito = 9;
            horaTemporal.minutos = (uint8_t)((horaTemporal.minutos / 10 * 10) + digito);
        break;
        
        case 4: // Decenas Segundo
            digito = (int8_t)(horaTemporal.segundos / 10);
            digito += valorHora;
            if (digito > 5) digito = 0;
            if (digito < 0) digito = 5;
            horaTemporal.segundos = (uint8_t)((digito * 10) + (horaTemporal.segundos % 10));
        break;
        
        case 5: // Unidades Segundo
            digito = (int8_t)(horaTemporal.segundos % 10);
            digito += valorHora;
            if (digito > 9) digito = 0;
            if (digito < 0) digito = 9;
            horaTemporal.segundos = (uint8_t)((horaTemporal.segundos / 10 * 10) + digito);
        break;
    }
    
    // Reinicio de las horas si superamos las 23:59
    if (horaTemporal.horas > 23) horaTemporal.horas = 23;
}
