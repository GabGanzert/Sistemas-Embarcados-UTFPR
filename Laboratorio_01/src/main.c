#include <stdint.h>
#include <stdbool.h>
// includes da biblioteca driverlib
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"

#define SYSTEM_FREQ 24000000
#define for_CLOCK_CYCLES 4 // for = 4 ciclos de clock cada iteração (na arquitetura Cortex-M4).


void main(void){
  uint32_t freq = SysCtlClockFreqSet(SYSCTL_OSC_INT | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, SYSTEM_FREQ);
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // Habilita GPIO F (LED D3 = PF4, LED D4 = PF0)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)); // Aguarda final da habilitação
    
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4); // LEDs D3 e D4 como saída
  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, 0); // LEDs D3 e D4 apagados
  GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);

  while(1){
      GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, !GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)); // Troca estado do LED D4
      for(uint32_t i=0;i<freq/for_CLOCK_CYCLES;i++);//delay ~= 1s        
  } // while
} // main