# EMBEBBED MP3 PLAYER ON STM32F429ZI
Este proyecto consiste en el diseño e implementación de un reproductor de audio embebido completo, desarrollado sobre el microcontrolador STM32F429ZI. La arquitectura del sistema es totalmente modular y está gestionada por un sistema operativo de tiempo real (RTOS).

### 🏗️ Arquitectura del Software
El sistema utiliza el estándar CMSIS-RTOS2 para la gestión de concurrencia, implementando una arquitectura de hilos cooperativos donde cada periférico cuenta con su propio hilo de ejecución dedicado.
- ***Multitarea y Sincronización:*** Uso intensivo de colas de mensajes (osMessageQueue), flags de eventos y hilos independientes para garantizar un funcionamiento fluido y asíncrono.
- ***Máquina de Estados (FSM):*** El núcleo del sistema (Thprincipal) implementa una FSM con tres estados principales: REPOSO, REPRODUCCIÓN y PROGRAMACIÓN DE HORA.

### 🔧 Periféricos e Interfaces
El firmware integra múltiples protocolos de comunicación y periféricos internos del microcontrolador:
- ***SPI (Serial Peripheral Interface):*** Control de la pantalla LCD para la interfaz de usuario, optimizando el refresco mediante buffers gráficos en RAM.
- ***I2C (Inter-Integrated Circuit):*** Adquisición de datos del sensor de temperatura ambiente LM75B.
- ***UART/USART:***\
  -- Control del módulo reproductor MP3 CATALEX (9600 baudios).\
  -- Generación de un log de depuración en tiempo real enviado al PC (115200 baudios) con marcas de tiempo.
- ***ADC (Analog-to-Digital Converter):*** Lectura analógica del potenciómetro para un control de volumen dinámico y preciso.
- ***PWM (Pulse Width Modulation):***\
  -- Generación de señales acústicas mediante el Timer 1 (Buzzer) para notificaciones del sistema.\
  -- Control de intensidad del LED RGB para la funcionalidad de Vúmetro (brillo proporcional a la amplitud de audio) mediante el Timer 4.

### 🚀 Funcionalidades Destacadas
- ***Gestión de Audio:*** Soporte para reproducción, pausa, navegación por carpetas/pistas y ajuste de volumen mediante hardware.
- ***Robustez:*** Filtro de rebotes (debouncing) para el joystick mediante interrupciones externas (EXTI) y timers dedicados.
- ***Ajuste de Hora:*** Sistema de programación de hora manual con cursor interactivo en el LCD sin detener el cronómetro del sistema.
- ***Funcionalidades Opcionales:*** Implementación de Reproducción Continua automática y visualización de niveles de audio.

### 🛠️ Herramientas Utilizadas
- ***Entorno de Desarrollo:*** Keil uVision 5.
- ***Librerías:*** STM32 HAL (Hardware Abstraction Layer) y CMSIS-RTOS2.
- ***Hardware:*** NUCLEO-F429ZI y MBED Application Board.

### 👥 Colaboradores
Proyecto académico realizado por:
- Raúl Torres.
- Paula Yago.
