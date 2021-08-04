#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS
#include "driverbuttons.h"

#define CHANGE_LED 0x01
#define INCREMENT_DUTY_CYCLE 0x02
#define PWM_PERIOD_MS 10
#define INITIAL_DUTY_CYCLE 10
#define QUEUE_SIZE 10

osThreadId_t controladora_id,
             acionadora_led1_id, acionadora_led2_id, acionadora_led3_id, acionadora_led4_id;

osMutexId_t mutex_id;

typedef struct{
  uint8_t led;
  uint8_t d_c;
  osMessageQueueId_t pwm_queue_id;
}led_pwm_t;

led_pwm_t led_pwm_thread1, led_pwm_thread2, led_pwm_thread3, led_pwm_thread4; 

//USR_SWs handler
void GPIOJ_Handler(void){
  if ((~ButtonRead(USW1) & USW1) == USW1)
  {
    ButtonIntClear(USW1);
    osThreadFlagsSet(controladora_id, CHANGE_LED);
  }
  if ((~ButtonRead(USW2) & USW2) == USW2)
  {
    ButtonIntClear(USW2);
    osThreadFlagsSet(controladora_id, INCREMENT_DUTY_CYCLE);
  }
} 

void controladora(void *arg)
{
  uint8_t ret = 0, count = 0, d_c[4]={INITIAL_DUTY_CYCLE, INITIAL_DUTY_CYCLE,
                                      INITIAL_DUTY_CYCLE, INITIAL_DUTY_CYCLE};
  osMessageQueueId_t msg_id = led_pwm_thread1.pwm_queue_id;
  
  while(1)
  {
    ret = (uint8_t)osThreadFlagsWait(CHANGE_LED | INCREMENT_DUTY_CYCLE, osFlagsWaitAny, osWaitForever);
    if ((ret & CHANGE_LED) == CHANGE_LED)
    {
      //logica para trocar tarefa acionadora
      count++;
      if(count >= 4) count=0;
      
      switch (count)
      {
        case 0:
          msg_id = led_pwm_thread1.pwm_queue_id;
        break;
        case 1:
          msg_id = led_pwm_thread2.pwm_queue_id;
        break;
        case 2:
          msg_id = led_pwm_thread3.pwm_queue_id;
        break;
        case 3:
          msg_id = led_pwm_thread4.pwm_queue_id;
        break;
        default:
        break;
      }      
    }
    if ((ret & INCREMENT_DUTY_CYCLE) == INCREMENT_DUTY_CYCLE)
    {
      //logica para incrementar duty cycle
      d_c[count] += 10;
      if (d_c[count] > 100)  d_c[count] = INITIAL_DUTY_CYCLE;
      osMessageQueuePut(msg_id, &d_c[count], 0, osWaitForever);
    }
  }
}

void acionadora(void *arg)
{
  led_pwm_t *led_pwm = (led_pwm_t *)arg;
  uint8_t state = 0;
  uint32_t tick = 0;
  
  while(1){
    osMutexAcquire(mutex_id, osWaitForever);
    //inicio seção crítica
    tick = osKernelGetTickCount(); 
    state ^= led_pwm->led;
    LEDWrite(led_pwm->led, state); 
    //fim seção crítica
    osMutexRelease(mutex_id);
    osDelayUntil(tick + (led_pwm->d_c * PWM_PERIOD_MS)/100); 
    

    osMutexAcquire(mutex_id, osWaitForever);
    //inicio seção crítica
    tick = osKernelGetTickCount(); 
    state ^= led_pwm->led;
    LEDWrite(led_pwm->led, state);
    //fim seção crítica
    osMutexRelease(mutex_id);
    osDelayUntil(tick + ((100 - led_pwm->d_c) * PWM_PERIOD_MS)/100); 
    
    osMessageQueueGet(led_pwm->pwm_queue_id, &led_pwm->d_c, NULL, 0); 
  } // while   
}

void main(void){
  SystemInit();
  
  LEDInit(LED4 | LED3 | LED2 | LED1);
  
  //---------------configs dos botoes------------------------
  ButtonInit(USW1 | USW2);
  ButtonIntEnable(USW1 | USW2);
  
  //inicializaçao das structs p/ cada thread
  
  led_pwm_thread1.led = LED1;
  led_pwm_thread1.d_c = INITIAL_DUTY_CYCLE;
  
  led_pwm_thread2.led = LED2;
  led_pwm_thread2.d_c = INITIAL_DUTY_CYCLE;
  
  led_pwm_thread3.led = LED3;
  led_pwm_thread3.d_c = INITIAL_DUTY_CYCLE;
  
  led_pwm_thread4.led = LED4;
  led_pwm_thread4.d_c = INITIAL_DUTY_CYCLE;
  
  osKernelInitialize();
  
  mutex_id = osMutexNew(NULL);

  led_pwm_thread1.pwm_queue_id = osMessageQueueNew(QUEUE_SIZE, sizeof(uint8_t), NULL);
  led_pwm_thread2.pwm_queue_id = osMessageQueueNew(QUEUE_SIZE, sizeof(uint8_t), NULL);
  led_pwm_thread3.pwm_queue_id = osMessageQueueNew(QUEUE_SIZE, sizeof(uint8_t), NULL);
  led_pwm_thread4.pwm_queue_id = osMessageQueueNew(QUEUE_SIZE, sizeof(uint8_t), NULL);
  
  controladora_id = osThreadNew(controladora, NULL, NULL);
  acionadora_led1_id = osThreadNew(acionadora, &led_pwm_thread1, NULL);
  acionadora_led2_id = osThreadNew(acionadora, &led_pwm_thread2, NULL);
  acionadora_led3_id = osThreadNew(acionadora, &led_pwm_thread3, NULL);
  acionadora_led4_id = osThreadNew(acionadora, &led_pwm_thread4, NULL);
    
  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main

