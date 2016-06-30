#include <18F26J50.h>
#device adc=10
#device HIGH_INTS=true
#FUSES INTRC_IO
#FUSES NOWDT
#FUSES NODEBUG
#FUSES NOXINST
#FUSES PROTECT
#FUSES NOFCMEN
#FUSES NOIESO
#FUSES NOCPUDIV

#use delay(internal=8000000)

//Uart 2
#pin_select TX2=PIN_B2
#pin_select RX2=PIN_B3

#use rs232(UART1, baud=9600, bits=8, parity=N, STOP=1, stream=uart1, errors) //Arduino
#use rs232(UART2,enable=PIN_A0, baud=9600, bits=8, parity=N, STOP=1, stream=uart2, errors) //Nodos
#use rs232(baud=9600,parity=N,xmit=PIN_C1,rcv=PIN_C0,bits=8,stream=uart3,errors) //Debugger USB
