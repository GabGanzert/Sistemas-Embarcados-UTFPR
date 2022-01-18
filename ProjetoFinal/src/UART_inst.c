#include <stdbool.h>
#include <stdio.h>
#include "cmsis_os2.h" // CMSIS-RTOS

// includes da biblioteca driverlib
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "utils/uartstdio.h"
#include "system_TM4C1294.h"


osThreadId_t thread1_id, thread2_id;
osMutexId_t uart_id;

const osThreadAttr_t thread1_attr = {
  .name = "Thread 1"
};

const osThreadAttr_t thread2_attr = {
  .name = "Thread 2"
};


//----------
// UART definitions
extern void UARTStdioIntHandler(void);

void UARTInit(void){
  // Enable UART0
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

  // Initialize the UART for console I/O.
  UARTStdioConfig(0, 115200, SystemCoreClock);

  // Enable the GPIO Peripheral used by the UART.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));

  // Configure GPIO Pins for UART mode.
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
} // UARTInit

void UART0_Handler(void){
  UARTStdioIntHandler();
} // UART0_Handler
//----------


void myKernelInfo(void){
  osVersion_t osv;
  char infobuf[16];
  if(osKernelGetInfo(&osv, infobuf, sizeof(infobuf)) == osOK) {
    UARTprintf("Kernel Information: %s\n",   infobuf);
    UARTprintf("Kernel Version    : %d\n",   osv.kernel);
    UARTprintf("Kernel API Version: %d\n\n", osv.api);
    UARTFlushTx(false);
  } // if
} // myKernelInfo

void myKernelState(void){
  UARTprintf("Kernel State: ");
  switch(osKernelGetState()){
    case osKernelInactive:
      UARTprintf("Inactive\n\n");
      break;
    case osKernelReady:
      UARTprintf("Ready\n\n");
      break;
    case osKernelRunning:
      UARTprintf("Running\n\n");
      break;
    case osKernelLocked:
      UARTprintf("Locked\n\n");
      break;
    case osKernelError:
      UARTprintf("Error\n\n");
      break;
  } //switch
  UARTFlushTx(false);
} // myKernelState

void myThreadState(osThreadId_t thread_id){
  osThreadState_t state;
  state = osThreadGetState(thread_id);
  switch(state){
  case osThreadInactive:
    UARTprintf("Inactive\n");
    break;
  case osThreadReady:
    UARTprintf("Ready\n");
    break;
  case osThreadRunning:
    UARTprintf("Running\n");
    break;
  case osThreadBlocked:
    UARTprintf("Blocked\n");
    break;
  case osThreadTerminated:
    UARTprintf("Terminated\n");
    break;
  case osThreadError:
    UARTprintf("Error\n");
    break;
  } // switch
} // myThreadState

void myThreadInfo(void){
  osThreadId_t threads[8];
  uint32_t number = osThreadEnumerate(threads, sizeof(threads));
  
  UARTprintf("Number of active threads: %d\n", number);
  for(int n = 0; n < number; n++){
    UARTprintf("  %s (priority %d) - ", osThreadGetName(threads[n]), osThreadGetPriority(threads[n]));
    myThreadState(threads[n]);
  } // for
  UARTprintf("\n");
  UARTFlushTx(false);
} // myThreadInfo


// osRtxIdleThread
__NO_RETURN void osRtxIdleThread(void *argument){
  (void)argument;
  
  while(1){
    //UARTprintf("Idle thread\n");
    asm("wfi");
  } // while
} // osRtxIdleThread

// osRtxTimerThread



__NO_RETURN void thread1(void *arg){
  uint32_t count = 0;
  
  // Na primeira execução reporta estado do kernel e tarefas ativas
  osMutexAcquire(uart_id, osWaitForever);
  myKernelState();
  myThreadInfo();
  osMutexRelease(uart_id);
  
  while(1){
    count++;
    
    osMutexAcquire(uart_id, osWaitForever);
    UARTprintf("%s, job %d beg.\n", osThreadGetName(osThreadGetId()), count);
    osMutexRelease(uart_id);
    
    for(int d = 0; d < 10000; d++); // atraso de software simulando C
    
    osMutexAcquire(uart_id, osWaitForever);
    UARTprintf("%s, job %d end.\n", osThreadGetName(osThreadGetId()), count);
    osMutexRelease(uart_id);

    osDelay(1000); // bloqueio simulando D
  } // while
} // thread1

__NO_RETURN void thread2(void *arg){
  uint32_t count = 0;
  uint32_t tick;
  
  while(1){
    tick = osKernelGetTickCount();
    
    count++;

    osMutexAcquire(uart_id, osWaitForever);
    UARTprintf("%s, job %d beg.\n", osThreadGetName(osThreadGetId()), count);
    osMutexRelease(uart_id);

    for(int d = 0; d < 10000; d++); // atraso de software simulando C

    osMutexAcquire(uart_id, osWaitForever);
    UARTprintf("%s, job %d end.\n", osThreadGetName(osThreadGetId()), count);
    osMutexRelease(uart_id);
   
    osDelayUntil(tick + 2000); // bloqueio simulando T
  } // while
} // thread2

void main(void){
  UARTInit();
  myKernelInfo();
  myKernelState();
  
  if(osKernelGetState() == osKernelInactive)
     osKernelInitialize();

  myKernelState();

  thread1_id = osThreadNew(thread1, NULL, &thread1_attr);
  thread2_id = osThreadNew(thread2, NULL, &thread2_attr);
  uart_id = osMutexNew(NULL);

  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main
