/**
SBM 25/26 - PROYECTO
Raúl Torres Huete
Paula Yago Alcol
(X08X09V10V11B)
  - Módulo encargado de la gestión del LCD
  */
  
	
/*PROTOTIPO DE LCD.H QUE SE REUTILIZARÁ, CONTIENE LAS LLAMADAS A LAS FUNCIONES RELACIONADAS AL LCD*/

#ifndef _LCD_
	
	#include "stm32f4xx_hal.h"
	#include "Driver_SPI.h"
	#include "stdio.h"


	#define _LCD_
  
    int Init_LCD(void);            //Función de inicialización del hilo
    void LCD (void *argument);     //Función del hilo 
	
    //Añadido en P3_1
    void initpinesLCD(void);
    void SPI_Init(void);
    void LCD_reset(void);

    //Añadido en P3_2
    void LCD_wr_data(unsigned char data);
    void LCD_wr_cmd(unsigned char cmd);

    //Añadido en P3_3
    void LCD_init(void);

    //Añadido en P3_4
    void LCD_update(void);
    void pintarColumnaLCD(uint8_t columna, uint8_t pagina, uint8_t valor);    // Función extra muy cómoda para pintar el lcd

    //Añadido en P3_5
    void symbolToLocalBuffer_L1(uint8_t symbol);              //Función para escribir un símbolo en la línea 1 del LCD
    void sentenceToLocalBuffer_L1(const char *frase);         //Función para escribir una frase en la línea 1 del LCD

    //Añadido en P3_6
    void symbolToLocalBuffer_L2(uint8_t symbol);              //Función para escribir un símbolo en la línea 2 del LCD
    void sentenceToLocalBuffer_L2(const char *frase);         //Función para escribir una frase en la línea 2 del LCD

    void symbolToLocalBuffer(uint8_t linea, uint8_t symbol);      //Función para escribir una frase en la línea que queramos del LCD
    void sentenceToLocalBuffer(uint8_t linea, const char *frase); //Función para escribir una frase en la línea que queramos del LCD

    void escribirLCD(char frase1[80], char frase2[80]);     //Función para escribir una oración cualquiera en las líneas que se digan del LCD 
                                //  (contemplando también variables, no solo texto)
    //Añadido en P5_3
    void borrarBuffer(void);    //Función para borrar el buffer
		
		extern ARM_DRIVER_SPI Driver_SPI1; //driver del protocolo SPI
    
    
    void escribirModoReposo(char frase1[80], char frase2[80]);
    void escribirModoReproduccion(char frase1[80], char frase2[80]);
    void escribirModoProgramacion(char frase1[80], char frase2[80]);
		
    
    void dibujarSubrayado(int8_t pos);

		
		typedef struct{
      uint8_t horaLCD;
      uint8_t minutosLCD;
      uint8_t segundosLCD;
      char frase1[80];
      char frase2[80];
      int8_t posCursor;
      uint8_t reproCont;
    } infoLCD;

#endif
