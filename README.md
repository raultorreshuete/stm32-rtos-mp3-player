# Reproductor MP3 Inteligente - STM32F429ZI
Diseño e implementación de un reproductor de audio embebido robusto gestionado por un sistema operativo de tiempo real (RTOS). El sistema permite la gestión de archivos multimedia, control de volumen y monitorización ambiental.

### 🔧 Hardware e Interfaces
El firmware integra múltiples protocolos de comunicación y periféricos internos del microcontrolador:
- ***Protocolo SPI:*** Comunicación con el display LCD para la interfaz de usuario.
- ***Protocolo I2C:*** Adquisición de temperatura ambiente mediante el sensor LM75B.
- ***Protocolo UART/USART:*** Control del módulo MP3 CATALEX y generación de logs para depuración en PC.
- ***Entrada Analógica (ADC):*** Lectura del potenciómetro para control de volumen dinámico.
- ***Salidas PWM:*** Generación de avisos acústicos (Timer 1) y control de brillo de LED RGB para vúmetro (Timer 4).

### 🏗️ Arquitectura de Software
- ***Sistema Operativo:*** Basado en CMSIS-RTOS2 para la gestión de hilos independientes (concurrencia) y sincronización mediante colas de mensajes.
- ***Lógica de Control:*** Implementación de una Máquina de Estados Finitos (FSM) con 3 modos: REPOSO, REPRODUCCIÓN y PROGRAMACIÓN DE HORA.
- ***Sincronización:*** Uso de Flags de eventos para la actualización precisa del cronómetro de canciones.

### 🚀 Funcionalidades Clave
- Control total de reproducción (Play, Pause, Next, Prev).
- Ajuste manual de la hora del sistema mediante joystick con cursor interactivo.
- Modo de reproducción continua automática.

### 🛠️ Herramientas Utilizadas
- ***Entorno:*** Keil uVision 5.
- ***Librerías:*** STM32 HAL y CMSIS-RTOS2.
- ***Hardware:*** NUCLEO-F429ZI y MBED Application Board.

### 👥 Colaboradores
Proyecto académico desarrollado por Raúl Torres y Paula Yago.
