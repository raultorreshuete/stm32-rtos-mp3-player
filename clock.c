/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión del reloj del sistema
  */

#include "cmsis_os2.h"
#include "clock.h"
#include "principal.h"

extern osThreadId_t tid_principal;

//Inicialización de un struct HoraCompleta, que está en el clock.h
HoraCompleta hora;

osThreadId_t tid_clock; //Id del hilo

//Función inicialización del hilo del reloj
int Init_clock (void) {
  
  //Reset del reloj
  hora.horas = 0;
  hora.minutos = 0;
  hora.segundos = 0;
  
  //Creación e iniciación del hilo del reloj
  tid_clock = osThreadNew(clock, NULL, NULL);
  if (tid_clock == NULL) {
    return(-1);
  }
  return(0);
}

//Función del hilo del reloj
void clock (void *argument) {
  
  while(1){
    
    osDelay(1000);			//Espera de 1 seg cada iteración
    
    hora.segundos++;    //Sumamos segundos en cada ejecución

    //Si los segundos llegan a 60, se resetean y sumamos un minuto
    if (hora.segundos > 59) {
        hora.segundos = 0;
        hora.minutos++;
    }
    //Si los minutos llegan a 60, se resetean y sumamos una hora
    if (hora.minutos > 59) {
        hora.minutos = 0;
        hora.horas = (hora.horas + 1) % 24;
    }
    //Si las horas llegan a 24, se resetean
    if(hora.horas > 23){
      hora.horas = 0;
    }
    
    // --- LO NUEVO: AVISAR AL PRINCIPAL ---
    // Enviamos el flag indicando que ha pasado 1 segundo exacto
    osThreadFlagsSet(tid_principal, FLAG_SEGUNDO_NUEVO);
  }
}

