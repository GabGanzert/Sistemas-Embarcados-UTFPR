#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "system_tm4c1294.h" 
#include "cmsis_os2.h" // CMSIS-RTOS

// includes da biblioteca driverlib
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "utils/uartstdio.h"
#include "system_TM4C1294.h"

#include "main.h"

/*---------------------FUNCOES DA UART--------------------*/
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
  
  if(UARTPeek('\r') >= 0)
  {
    osThreadFlagsSet(receptora_id, RX_FLAG); 
  }
  
} // UART0_Handler
//----------

/*--------------------------CORPO DAS THREADS AQUI---------------------------*/



/*-----------------RECEPTORA-------------------*/
void receptora(void *arg)
{
  char first;
  char rx_buffer[UART_BUFFER_SIZE], tx_msg[UART_BUFFER_SIZE];
  elevador_t* recept_elev;
  
  while(1)
  {
    osThreadFlagsWait(RX_FLAG, osFlagsWaitAny, osWaitForever); 
    
    UARTgets(rx_buffer, UART_BUFFER_SIZE);
    
    UARTFlushRx();
    
    if(strcmp(rx_buffer, "initialized") == 0 && !INITIALIZED)
    {
      osThreadFlagsSet(gerenciadora_id, INIT_GERENCI_FLAG);
    }
    else if(INITIALIZED)
    {
      
      first = rx_buffer[0];
      
      if(first == 'e' || first == 'c' || first == 'd')
      {        
        if(rx_buffer[1] == 'E') //externa - coloca na gerenciadora
        {
          osMessageQueuePut(gerenciadora_queue_id, rx_buffer, 1, osWaitForever);
        }
        else if(rx_buffer[1] == 'I') //interno - coloca direto na fila do elev.
        {
          uint8_t prio = 0;
          char andar_char =  rx_buffer[2];
          uint8_t andar_int = andar_char - ASCII_FLOOR_CHAR_TO_INT;

          tx_msg[0] = SOLICITATION_CHAR; //indica q a msg e referente a solicitacao
          tx_msg[1] = first;
          
          if(andar_char < 'k') //andar menor q o 10: so tem 1 digito
          {
            tx_msg[2] = andar_char - ASCII_OFFSET_FLOOR_CHAR_TO_DIGIT_CHAR;
            tx_msg[3] = andar_int;
            tx_msg[4] = 0;
          }
          else // andar tem dois digitos
          {
            tx_msg[2] =  '1';
            tx_msg[3] = andar_char - (ASCII_OFFSET_FLOOR_CHAR_TO_DIGIT_CHAR + 10);
            tx_msg[4] = andar_int;
            tx_msg[5] = 0;  
          }
                    
          switch(first)
          {
            case 'e':
              recept_elev = &elev_esquerdo;
            break;
            case 'c':
              recept_elev = &elev_central;
            break;
            case 'd':
              recept_elev = &elev_direito;
            break;
            default:
            break;
          }
            
          int8_t diff_floors = recept_elev->andar - andar_int;

          prio = calc_prio(recept_elev->estado, diff_floors);

          recept_elev->requests++;
          
          if(prio > recept_elev->current_prio)
          {
            recept_elev->change_prio = true;   
          }
          
          osMessageQueuePut(recept_elev->mov_queue_id, tx_msg, prio, osWaitForever);   
        }
        else
        {
          switch(first)
          {
            case 'e':
              recept_elev = &elev_esquerdo;
            break;
            case 'c':
              recept_elev = &elev_central;
            break;
            case 'd':
              recept_elev = &elev_direito;
            break;
            default:
            break;
          }
          
          osMessageQueuePut(recept_elev->elev_queue_id, rx_buffer, 0, osWaitForever);
        }
      }
      else
      {
        switch(req_elev_pos)
        {
          case 'e':
            osMessageQueuePut(elev_esquerdo.elev_queue_id, rx_buffer, 0, osWaitForever);
          break;
          case 'c':
            osMessageQueuePut(elev_central.elev_queue_id, rx_buffer, 0, osWaitForever);
          break;
          case 'd':
            osMessageQueuePut(elev_direito.elev_queue_id, rx_buffer, 0, osWaitForever);
          break;
        }
      }
    }
    
    for(uint8_t i = 0; i < UART_BUFFER_SIZE; i++)
    {
      rx_buffer[i] = 0;
    }
  }
}


/*-----------------GERENCIADORA-------------------*/

void gerenciadora(void *arg)
{
  char rx_msg[BUFFER_SIZE] = {0}, tx_msg[BUFFER_SIZE] = {0};
  uint8_t andar_request = 0;
  char dir;
  elevador_t* gerenci_elev;
  
  osThreadFlagsWait(INIT_GERENCI_FLAG, osFlagsWaitAny, osWaitForever);
  
  INITIALIZED = true;
  
  osThreadFlagsSet(elev_esquerdo.id, INIT_ACIONA_FLAG);
  osThreadFlagsSet(elev_central.id, INIT_ACIONA_FLAG);
  osThreadFlagsSet(elev_direito.id, INIT_ACIONA_FLAG);  
  
  while(1)
  {
    osMessageQueueGet(gerenciadora_queue_id, &rx_msg, NULL, osWaitForever);
    
    andar_request = ((rx_msg[2] - ASCII_OFFSET) * 10) + (rx_msg[3] - ASCII_OFFSET);
    dir = rx_msg[4];
    
    switch(rx_msg[0])
    {
      case 'e': // solicitacao externa no botao elevador ESQUERDO        
        if(dir == 'd') //PARA DESCER
        {
          if( ((elev_esquerdo.andar > andar_request) && (elev_esquerdo.estado == DESCENDO))
             || elev_esquerdo.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_esquerdo;
          }
          else if( ((elev_central.andar > andar_request) && (elev_central.estado == DESCENDO))
             || elev_central.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_central;
          }
          else if( ((elev_direito.andar > andar_request) && (elev_direito.estado == DESCENDO))
             || elev_direito.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_direito;
          }
        }
        else if (dir == 's') //PARA SUBIR
        {
          if( ((elev_esquerdo.andar < andar_request) && (elev_esquerdo.estado == SUBINDO))
             || elev_esquerdo.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_esquerdo;
          }
          else if( ((elev_central.andar < andar_request) && (elev_central.estado == SUBINDO))
             || elev_central.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_central;
          }
          else if( ((elev_direito.andar < andar_request) && (elev_direito.estado == SUBINDO))
             || elev_direito.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_direito;
          }
        }
      break;
      case 'c': // solicitacao externa no botao elevador central
        if(dir == 'd') //PARA DESCER
        {
          if( ((elev_central.andar > andar_request) && (elev_central.estado == DESCENDO))
             || elev_central.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_central;
          }
          else if( ((elev_esquerdo.andar > andar_request) && (elev_esquerdo.estado == DESCENDO))
             || elev_esquerdo.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_esquerdo;
          }
          else if( ((elev_direito.andar > andar_request) && (elev_direito.estado == DESCENDO))
             || elev_direito.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_direito;
          }
        }
        else if (dir == 's') //PARA SUBIR
        {
          if( ((elev_central.andar < andar_request) && (elev_central.estado == SUBINDO))
             || elev_central.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_central;
          }
          else if( ((elev_esquerdo.andar < andar_request) && (elev_esquerdo.estado == SUBINDO))
             || elev_esquerdo.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_esquerdo;
          }
          else if( ((elev_direito.andar < andar_request) && (elev_direito.estado == SUBINDO))
             || elev_direito.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_direito;
          }
        }        
      break;
      case 'd': // solicitacao externa no botao elevador direito        
        if(dir == 'd') //PARA DESCER
        {
          if( ((elev_direito.andar > andar_request) && (elev_direito.estado == DESCENDO))
             || elev_direito.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_direito;
          }
          else if( ((elev_central.andar > andar_request) && (elev_central.estado == DESCENDO))
             || elev_central.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_central;
          }
          else if( ((elev_esquerdo.andar > andar_request) && (elev_esquerdo.estado == DESCENDO))
             || elev_esquerdo.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_esquerdo;
          }
        }
        else if (dir == 's') //PARA SUBIR
        {
          if( ((elev_direito.andar < andar_request) && (elev_direito.estado == SUBINDO))
             || elev_direito.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_direito;
          }
          else if( ((elev_central.andar < andar_request) && (elev_central.estado == SUBINDO))
             || elev_central.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_central;
          }
          else if( ((elev_esquerdo.andar < andar_request) && (elev_esquerdo.estado == SUBINDO))
             || elev_esquerdo.estado == PARADO_ABERTO)
          {
            gerenci_elev = &elev_esquerdo;
          }
        }        
      break;
      default:
      break;
    }
    
    int8_t diff_floors = gerenci_elev->andar - andar_request;
    uint8_t prio = 0;

    if(dir == 'd')
    {
      prio = calc_prio(DESCENDO, diff_floors);
    }
    else if(dir == 's')
    {
      prio = calc_prio(SUBINDO, diff_floors);
    }
    
    tx_msg[0] = SOLICITATION_CHAR;
    tx_msg[1] = gerenci_elev->elev_ch;
    
    if(andar_request < 10)
    {
      tx_msg[2] = andar_request + ASCII_OFFSET;
      tx_msg[3] = andar_request;
      tx_msg[4] = 0;
    }
    else
    {
      tx_msg[2] = '1';
      tx_msg[3] = (andar_request % 10) + ASCII_OFFSET;
      tx_msg[4] = andar_request;
      tx_msg[5] = 0;  
    }
    
    gerenci_elev->requests++;
    
    if(prio > gerenci_elev->current_prio)
    {
      gerenci_elev->change_prio = true;   
    }
    
    osMessageQueuePut(gerenci_elev->mov_queue_id, tx_msg, prio, osWaitForever);    
  }
}


/*-----------------ACIONADORA-------------------*/

void acionadora(void *arg)
{
  elevador_t* elev = (elevador_t *) arg;
  
  char rx_msg[BUFFER_SIZE] = {0}, tx_msg[BUFFER_SIZE] = {0},
      exp_msg[BUFFER_SIZE] = {0}, exp_msg_floor[BUFFER_SIZE] = {0};
  
  uint8_t andar_request = 0, prio_recv = 0;
  int dif = 0, position_to_reach = 0, last_position = 0;
  
  bool come_to_destiny = false, on_previous_floor = false;
  
  osThreadFlagsWait(INIT_ACIONA_FLAG, osFlagsWaitAny, osWaitForever);
  
  osMutexAcquire(mutex_id, osWaitForever);
  UARTprintf("%c%c\r", elev->elev_ch, 'r'); //inicializa elevador
  osMutexRelease(mutex_id);
  
  elev->estado = PARADO_ABRINDO;
  sprintf(exp_msg, "%cA\r", elev->elev_ch);
  
  while(1)
  {
    
    if(elev->change_prio)   // chegou sol. com maior prioridade
    {
      if(elev->requests > 1) //se houver mais de uma solicitacao, coloca a atual de volta na fila
      {
        sprintf(tx_msg, "S%s", exp_msg_floor);
        
        osMessageQueuePut(elev->mov_queue_id, tx_msg, elev->current_prio, osWaitForever);
      }
      
      //recebe a solicitacao de maior prioridade
      osMessageQueueGet(elev->mov_queue_id, rx_msg,  &elev->current_prio, 0);
      
      elev->change_prio = false;
      
      if(rx_msg[0] == SOLICITATION_CHAR) //e uma solicitacao
      {
        //obtem o andar solicitado
        if(rx_msg[4] == 0) //andar so tem 1 digito
        {
          andar_request = rx_msg[3];
        }
        else
        {
          andar_request = rx_msg[4];
        }

        // obtem posicao a ser alcancada
        position_to_reach = (MAX_POSITION/(QTT_LEVELS - 1)) * andar_request;

        parse_solicitation(elev, andar_request, rx_msg, exp_msg_floor);
      }
      
      //se o elevador ja estiver em movimento, basta substituir exp_msg.
      //se nao, primeiro fecha a porta caso esteja aberta (esperando confirmacao),
      //e verifica a diferenca do andar atual do elevador e o andar solicitado:
      //se for negativa, deve subir, se for positiva deve descer.
      
      if(elev->estado == SUBINDO || elev->estado == DESCENDO)
      {
        strcpy(exp_msg, exp_msg_floor);
      }
      else if(elev->estado == PARADO_ABERTO && elev->andar == 0)
      {
        osDelay(DELAY_MS_TO_WAIT_TO_CLOSE);
        
        if(andar_request == 1)
        {
          on_previous_floor = true;
        }
        
        osMutexAcquire(mutex_id, osWaitForever);
        UARTprintf("%c%c\r", elev->elev_ch, 'f'); //pediu pra fechar a porta
        osMutexRelease(mutex_id);
        
        elev->estado = PARADO_FECHANDO;
        sprintf(exp_msg, "%cF", elev->elev_ch);     
      }
    }
  
    //deve atualizar a posicao atual do elevador
    if(elev->estado == SUBINDO || elev->estado == DESCENDO)
    {
      if(on_previous_floor)
      { 
        if(elev->current_position < (position_to_reach - (TOLERANCE_BASE + (andar_request * FLOOR_FACTOR)))
           || elev->current_position > (position_to_reach + (TOLERANCE_BASE + (andar_request * FLOOR_FACTOR))) )
        {         
            osDelay(DELAY_MS_TO_ADJUST);
          
            get_position(elev, rx_msg);
        }
        else
        {          
          osMutexAcquire(mutex_id, osWaitForever);
          UARTprintf("%c%c\r", elev->elev_ch, 'p'); //pede p/ parar
          osMutexRelease(mutex_id);
          
          osDelay(DELAY_MS_TO_STOP);
          
          osMutexAcquire(mutex_acert, osWaitForever);
          elev->estado = ACERTA_POSICAO;
        }
      }
      else
      {
        osMessageQueueGet(elev->elev_queue_id, rx_msg,  NULL, 0);
      }
    }

    //----------------MAQUINA DE ESTADOS--------------------
    switch(elev->estado)
    {
    //------------------------------------------------------  
      case PARADO_FECHADO:
        if(elev->requests > 0)
        {
          if(!elev->change_prio)
          { 
            osMessageQueueGet(elev->mov_queue_id, rx_msg, &prio_recv, 0);
            
            if(prio_recv == 0)
            {
              osMessageQueuePut(elev->elev_queue_id, rx_msg,  0, osWaitForever); 
            }
            else
            {
              elev->current_prio = prio_recv;
              
              if(rx_msg[0] == SOLICITATION_CHAR) //e uma solicitacao
              {                
                if(rx_msg[4] == 0) //andar so tem 1 digito
                {
                  andar_request = rx_msg[3];     
                }
                else
                {
                  andar_request = rx_msg[4];
                }
                
                parse_solicitation(elev, andar_request, rx_msg, exp_msg_floor);

                position_to_reach = (MAX_POSITION/(QTT_LEVELS - 1)) * andar_request;
              }
            }
          }
          
          dif = elev->andar - andar_request;
          
          if(dif < 0)
          {
            osMutexAcquire(mutex_id, osWaitForever);
            UARTprintf("%c%c\r", elev->elev_ch, 's'); //pediu subir
            osMutexRelease(mutex_id);
            
            elev->estado = SUBINDO;
            
            strcpy(exp_msg, exp_msg_floor);
          }
          else if(dif > 0)
          {
             osMutexAcquire(mutex_id, osWaitForever);
             UARTprintf("%c%c\r", elev->elev_ch, 'd'); //pediu descer
             osMutexRelease(mutex_id); 
             
             elev->estado = DESCENDO;
             
             strcpy(exp_msg, exp_msg_floor);
          }
        }
        else if(elev->andar != 0)
        {          
          sprintf(tx_msg, "%c%c%c", SOLICITATION_CHAR, elev->elev_ch, '0');
          tx_msg[3] = 0;
          osMessageQueuePut(elev->mov_queue_id, tx_msg,  1, osWaitForever);
          elev->requests++;
        }
      break;
      //------------------------------------------------------ 
      case PARADO_ABRINDO:
        osMessageQueueGet(elev->elev_queue_id, rx_msg, &prio_recv, osWaitForever);

        if(strncmp(exp_msg, rx_msg, 2) == 0)
        {
          elev->estado = PARADO_ABERTO; 
        }
      break;
      //------------------------------------------------------ 
      case PARADO_ABERTO:
        if(come_to_destiny)
        {
          come_to_destiny = false;
          on_previous_floor = false;
          elev->requests--;
          elev->andar = andar_request;
          
          osDelay(7000);
          
          if(elev->andar != 0)
          {
            osMutexAcquire(mutex_id, osWaitForever);
            UARTprintf("%c%c\r", elev->elev_ch, 'f'); //pediu pra fechar a porta
            osMutexRelease(mutex_id);
            
            elev->estado = PARADO_FECHANDO;
            sprintf(exp_msg, "%cF", elev->elev_ch);
          }  
        }
      break;
      //------------------------------------------------------ 
      case PARADO_FECHANDO:
        osMessageQueueGet(elev->elev_queue_id, rx_msg, &prio_recv, osWaitForever); 

        if(strncmp(exp_msg, rx_msg, 2) == 0)
        {
          elev->estado = PARADO_FECHADO; 
        }
      break;
      //------------------------------------------------------ 
      case SUBINDO:
        if((andar_request - elev->andar)  == 1)
        {
          on_previous_floor = true;
        }

        if(andar_request < 11)
        {
          if((strncmp(exp_msg, rx_msg, 2) == 0))
          {            
            on_previous_floor = true;
          }        
        }
        else
        {
          if((strncmp(exp_msg, rx_msg, 3) == 0))
          {
            on_previous_floor = true;
          }
        }
      break;
      //------------------------------------------------------ 
      case DESCENDO:
        if((elev->andar - andar_request) == 1)
        {
          on_previous_floor = true;
        }

        if(andar_request < 9)
        {
          if((strncmp(exp_msg, rx_msg, 2) == 0))
          {
            on_previous_floor = true;
          }          
        }
        else
        {
          if((strncmp(exp_msg, rx_msg, 3) == 0))
          {
            on_previous_floor = true;
          }
        }
      break; 
      case ACERTA_POSICAO:
                
        do
        {  
          last_position = elev->current_position;
          osDelay(2*DELAY_MS_TO_ADJUST);
          if(!get_position(elev, rx_msg))
          {
            get_position(elev, rx_msg);
          }
        }while(elev->current_position != last_position);
        
        if(elev->current_position < (position_to_reach - SIM_TOLERANCE))
        { 
          osMutexAcquire(mutex_id, osWaitForever);
          UARTprintf("%c%c\r", elev->elev_ch, 's'); //pediu subir
          
          osDelay(DELAY_MS_TO_ADJUST);

          UARTprintf("%c%c\r", elev->elev_ch, 'p'); //pede p/ parar
          osMutexRelease(mutex_id);
          
          osDelay(DELAY_MS_TO_STOP);
        }
        else if(elev->current_position > (position_to_reach + SIM_TOLERANCE))
        {         
          osMutexAcquire(mutex_id, osWaitForever);
          UARTprintf("%c%c\r", elev->elev_ch, 'd'); //pediu descer
          
          osDelay(DELAY_MS_TO_ADJUST);
          
          UARTprintf("%c%c\r", elev->elev_ch, 'p'); //pede p/ parar
          osMutexRelease(mutex_id);
          
          osDelay(DELAY_MS_TO_STOP);
        }
        else
        {       
          if(elev->current_position > (position_to_reach - SIM_TOLERANCE) 
             && elev->current_position < (position_to_reach + SIM_TOLERANCE))
          {
            osMutexRelease(mutex_acert);
            
            osMutexAcquire(mutex_id, osWaitForever);
            UARTprintf("%c%c\r", elev->elev_ch, 'a'); //pede p/ abrir
            osMutexRelease(mutex_id);
            
            elev->estado = PARADO_ABRINDO;
            sprintf(exp_msg, "%cA\r", elev->elev_ch);
            
            come_to_destiny = true;
          }
        }
      break;
      default:
      break;
    } 
 
  }
  
}


/*------------------------MAIN------------------------*/

void main(void){
  SystemInit();
  UARTInit();
  
  //inicializacao das structs p/ cada thread
  
  elev_esquerdo.elev_ch = 'e';
  elev_esquerdo.andar = 0;
  elev_esquerdo.estado = PARADO_FECHADO;
  elev_esquerdo.current_prio = 0;
  elev_esquerdo.requests = 0;
  elev_esquerdo.change_prio = false;
  elev_esquerdo.current_position = 0;
  
  elev_central.elev_ch = 'c';
  elev_central.andar = 0;
  elev_central.estado = PARADO_FECHADO;
  elev_central.current_prio = 0;
  elev_central.requests = 0;
  elev_central.change_prio = false;
  elev_central.current_position = 0;
  
  elev_direito.elev_ch = 'd';
  elev_direito.andar = 0;
  elev_direito.estado = PARADO_FECHADO;
  elev_direito.current_prio = 0;
  elev_direito.requests = 0;
  elev_direito.change_prio = false;
  elev_direito.current_position = 0;
  
  osKernelInitialize();

  //msg queues
  elev_esquerdo.elev_queue_id = osMessageQueueNew(QUEUE_SIZE, BUFFER_SIZE * sizeof(char), NULL);  
  elev_esquerdo.mov_queue_id = osMessageQueueNew(5, 8 * sizeof(char), NULL);
  
  elev_central.elev_queue_id = osMessageQueueNew(QUEUE_SIZE, BUFFER_SIZE * sizeof(char), NULL);  
  elev_central.mov_queue_id = osMessageQueueNew(5, 8 * sizeof(char), NULL);
  
  elev_direito.elev_queue_id = osMessageQueueNew(QUEUE_SIZE, BUFFER_SIZE * sizeof(char), NULL);
  elev_direito.mov_queue_id = osMessageQueueNew(5, 8 * sizeof(char), NULL);
  
  gerenciadora_queue_id = osMessageQueueNew(QUEUE_SIZE, BUFFER_SIZE * sizeof(char), NULL);
  
  //mutex
  mutex_id = osMutexNew(&Thread_Mutex_attr);
  mutex_acert = osMutexNew(&Thread_Mutex_attr);
   
  //threads
  receptora_id = osThreadNew(receptora, NULL, &thread1_attr_recpt);
  
  gerenciadora_id = osThreadNew(gerenciadora, NULL, &thread1_attr);
  
  elev_esquerdo.id = osThreadNew(acionadora, &elev_esquerdo, &thread1_attr);
  elev_central.id = osThreadNew(acionadora, &elev_central, &thread1_attr);
  elev_direito.id = osThreadNew(acionadora, &elev_direito, &thread1_attr);
  
  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main

bool get_position(elevador_t* elev, char* rx_msg)
{
	osMutexAcquire(mutex_id, osWaitForever);
    
    bool ret = false;
    req_elev_pos = elev->elev_ch;
    UARTprintf("%c%c\r", elev->elev_ch, 'x');
     
    osStatus_t q_ret = osMessageQueueGet(elev->elev_queue_id, rx_msg, NULL, 200);  
    
    if(q_ret == osOK)
    {  
      if(rx_msg[0] != 'e' && rx_msg[0] != 'c' && rx_msg[0] != 'd')
      {
        elev->current_position = atoi(rx_msg);
        ret = true;
      }
    } 
    
    osMutexRelease(mutex_id);
  
    return ret;
}

void parse_solicitation(elevador_t* elev, uint8_t andar_request, char* rx_msg, char* exp_msg_floor)
{
	int8_t dif = elev->andar - andar_request;
        
  if(andar_request < 10) //andar so tem 1 digito
  {          
    if(dif < 0)//deve subir
    {
        sprintf(exp_msg_floor, "%c%c%c", elev->elev_ch, rx_msg[2] - 1, rx_msg[3]);
    }
    else if (dif > 0)
    {
        if(andar_request == 9)
        {
          sprintf(exp_msg_floor, "%c%c%c%c", elev->elev_ch, rx_msg[2] - 8, rx_msg[2] - 9, rx_msg[3]);
        }
        else
        {  
          sprintf(exp_msg_floor, "%c%c%c", elev->elev_ch, rx_msg[2] + 1, rx_msg[3]);
        }
    }
  }
  else if(andar_request == 10)
  {
    if(dif < 0)//deve subir
    {
        sprintf(exp_msg_floor, "%c%c%c", elev->elev_ch, rx_msg[3] + 9, rx_msg[4]);
    }
    else if (dif > 0)
    {
        sprintf(exp_msg_floor, "%c%c%c%c", elev->elev_ch, rx_msg[2], rx_msg[3] +1, rx_msg[4]);
    }
  }
  else
  {
    if(dif < 0)//deve subir
    {
        sprintf(exp_msg_floor, "%c%c%c%c", elev->elev_ch, rx_msg[2], rx_msg[3] - 1, rx_msg[4]);
    }
    else if (dif > 0)
    {
        sprintf(exp_msg_floor, "%c%c%c%c", elev->elev_ch, rx_msg[2], rx_msg[3] + 1, rx_msg[4]);
    } 
  }
}

uint8_t calc_prio(estados estado, int8_t diff_floors)
{
  uint8_t prio = 0;

  if(estado == DESCENDO)
  {
    if(diff_floors < 0)
    {
      prio = diff_floors * -1;
    }
    else prio =  QTT_LEVELS - diff_floors;
  }
  else if (estado == SUBINDO)
  {
    if(diff_floors >= 0)
    {
      prio = diff_floors;
    }
    else prio =  QTT_LEVELS + diff_floors; 
  }
  else
  {
    if(diff_floors < 0)
    {
      prio = QTT_LEVELS + diff_floors;
    }
    else if(diff_floors > 0)
    { 
      prio = QTT_LEVELS - diff_floors;
    }
    else prio = 0;
  }

  return prio;  
}