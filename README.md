# EmbeddedDistributedGeneticAlgorithm

This is the source code of our project "A proposal of embedded distributed genetic algorithm for low-power, low-cost and low-size-memory devices". 

It consists of a distributed genetic algorithm implementation targeting embedded devices, such as microcontrollers.

You need to open this project on Atmel Studio 7.

### Running It

This project was initially developed using the IDE Atmel Studio 7.0 focusing the microcontroller ATmega328P - the same one present in Arduino Uno. 
Be aware that some features like sending text via USART for debug purposes depends on the device/ architecture you are using, therefore you may need
to use a different method. 

Moreover, if you want to debug and receive some float values via USART (using some C function such as `sprintf`, for example), 
you need to link the C library `printf_flt`. You can find a guide in the following link about how to do it using Atmel Studio 7, for example:

https://startingelectronics.org/articles/atmel-AVR-8-bit/print-float-atmel-studio-7/

### GA Parameters

All genetic algorithm parameters must be defined in the file `ga.h`. The only exception is the evaluation function, which is defined in the `main.c` file.
Please, notice that if you are using an evaluation function with multiple dimensions, you also need to change the parameter `DIMENSIONS`.

Also, the mater node is the device is `NODE_ID` 0.  All other devices are slaves.

### Pins Configuration

This project uses SPI as the interface to allow the communication of both microntrollers.

- Microcontroler ATmega328P Level

    - PB5 <----------> PB5  (SCK)
    - PB4 <----------> PB4  (MISO)  
    - PB3 <----------> PB3  (MOSI)
    - PB2 <----------> PB2  (SS2)
    - PB1 <----------> PB2  (SS1) - optional when using only 2 devices
    - PB0 <----------> PB2  (SS0) - optional when using only 2 devices

- Arduino Uno Level

    - Pin 13  <---------->  Pin 13  (SCK)
    - Pin 12  <---------->  Pin 12  (MISO)
    - Pin 11  <---------->  Pin 11  (MOSI)
    - Pin 10  <---------->  Pin 10  (SS2)
    - Pin 09  <---------->  Pin 09  (SS1) - optional when using only 2 devices
    - Pin 08  <---------->  Pin 08  (SS0) - optional when using only 2 devices
      
### Debugging and Evaluating Performance

If you want to check the result of the GA run, you need to send data via USART to your computer. The project already provides some functions that may be useful for you.

Finally, if you want to analyze the GA performance, you can use one of the internal counters (there are some functions ready to be used) or you can use one of the GPIO
pins to keep toggling between high and low voltage so you can use an external device (such as an oscilloscope) to measure the period.      
