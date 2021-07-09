#include <stdint.h>
#include <stdbool.h>
// includes da biblioteca driverlib
#include "driverlib/systick.h"
#include "driverleds.h" // Projects/drivers

// Seleção por evento

typedef enum {S_000, S_001, S_011, S_010, S_110, S_111, S_101, S_100} state_t;

volatile uint8_t Evento = 0;

void SysTick_Handler(void){
  Evento = 1;
} // SysTick_Handler

void main(void){
  static state_t Estado = S_000; // estado inicial da MEF
  
  LEDInit(LED1 | LED2 | LED3);
  SysTickPeriodSet(12000000); // f = 1Hz para clock = 24MHz
  SysTickIntEnable();
  SysTickEnable();

  while(1){
    if(Evento){
      Evento = 0;
      switch(Estado){
        case S_000:
          LEDOff(LED1 | LED2 | LED3);
          Estado = S_001;
        break;
        case S_001:
          LEDOff(LED1 | LED2);
          LEDOn(LED3);
          Estado = S_011;
        break;
        case S_011:
          LEDOff(LED1 );
          LEDOn(LED2 | LED3);
          Estado = S_010;
        break;
        case S_010:
          LEDOff(LED1 | LED3);
          LEDOn(LED2);
          Estado = S_110;
        break;
        case S_110:
          LEDOff(LED3);
          LEDOn(LED1 | LED2);
          Estado = S_111;
        break;
        case S_111:
          LEDOn(LED1 | LED2 | LED3);
          Estado = S_101;
        break;
        case S_101:
          LEDOff(LED2);
          LEDOn(LED1 | LED3);
          Estado = S_100;
        break;
        case S_100:
          LEDOff(LED2 | LED3);
          LEDOn(LED1);
          Estado = S_000;
        break;
        default:
        break;
      } // switch
    } // if
  } // while
} // main
