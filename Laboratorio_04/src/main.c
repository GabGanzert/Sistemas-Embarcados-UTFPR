#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS

#define LED_T1_PERIOD 200
#define LED_T2_PERIOD 300
#define LED_T3_PERIOD 500
#define LED_T4_PERIOD 700

osThreadId_t thread1_id, thread2_id, thread3_id, thread4_id;

typedef struct{
  uint8_t led;
  uint16_t period;
}led_struct;

void thread_led(void *arg){
  uint8_t state = 0;
  uint32_t tick;
  led_struct *led_t_struct = (led_struct *)arg;
  
  while(1){
    tick = osKernelGetTickCount(); 
    
    state ^= (*led_t_struct).led;
    LEDWrite((*led_t_struct).led, state); 
    
    osDelayUntil(tick + (*led_t_struct).period); 
  } // while    
} // thread

void main(void){
  LEDInit(LED4 | LED3 | LED2 | LED1);

  osKernelInitialize();
  
  led_struct led_thread1, led_thread2, led_thread3, led_thread4;
  
  led_thread1.led =  LED1;
  led_thread1.period = LED_T1_PERIOD;
  
  led_thread2.led =  LED2;
  led_thread2.period = LED_T2_PERIOD;
  
  led_thread3.led =  LED3;
  led_thread3.period = LED_T3_PERIOD;
  
  led_thread4.led =  LED4;
  led_thread4.period = LED_T4_PERIOD;
  
  

  thread1_id = osThreadNew(thread_led, &led_thread1, NULL);
  thread2_id = osThreadNew(thread_led, &led_thread2, NULL);
  thread3_id = osThreadNew(thread_led, &led_thread3, NULL);
  thread4_id = osThreadNew(thread_led, &led_thread4, NULL);

  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main