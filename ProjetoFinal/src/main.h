#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <stdbool.h>

#define QTT_LEVELS 16 //quantidade de andares com o terreo
#define MAX_POSITION 75000

#define QUEUE_SIZE 10 //msg queue size
#define UART_BUFFER_SIZE 15
#define BUFFER_SIZE 10

#define SOLICITATION_CHAR 'S'

#define ASCII_OFFSET_INT_NUM_TO_CHAR_NUM 48 //offset entre numero e char
#define ASCII_OFFSET_CHAR_NUM_TO_FLOOR_CHAR 49
#define ASCII_FLOOR_CHAR_TO_INT 97 //offset entre char que representa o andar e seu valor inteiro

#define SIM_TOLERANCE 100
#define TOLERANCE_BASE 500
#define FLOOR_FACTOR 1

#define DELAY_MS_TO_STOP 50
#define DELAY_MS_TO_ADJUST 200
#define DELAY_MS_TO_ADJUST_PRIO 10
#define DELAY_MS_TO_WAIT_TO_CLOSE 2000
#define DELAY_MS_TO_KEEP_OPEN 7000

//flags
enum{
  RX_FLAG = 0x01,
  INIT_GERENCI_FLAG = 0x02, 
  INIT_ACIONA_FLAG = 0x04
};

//estados do elevador
typedef enum{
  PARADO_ABERTO,
  PARADO_FECHANDO,
  PARADO_ABRINDO,
  PARADO_FECHADO,
  SUBINDO,
  DESCENDO,
  ACERTA_POSICAO
}estados;

//struct do elevador
typedef struct{
  osThreadId_t id;
  char elev_ch;
  estados estado;
  uint8_t andar;
  uint8_t current_prio;
  uint8_t requests;
  int current_position;
  bool change_prio;
  osMessageQueueId_t elev_queue_id;
  osMessageQueueId_t mov_queue_id;
}elevador_t;

typedef struct{
  char andar[3];
  int prio;
}request_t;

elevador_t elev_esquerdo, elev_central, elev_direito;

osThreadId_t receptora_id, gerenciadora_id;

osMutexId_t mutex_id, mutex_acert;

osMessageQueueId_t gerenciadora_queue_id;

bool INITIALIZED = false;

char req_elev_pos;

const osThreadAttr_t thread1_attr_recpt = {
  .priority = osPriorityRealtime  
};

const osThreadAttr_t thread1_attr = {
  .priority = osPriorityHigh
};

const osMutexAttr_t Thread_Mutex_attr = {
  .attr_bits = osMutexRecursive
};
 
 /**     
  * Gets elevator position 
  * 
  * @param  elev  pointer to elevator to get position.
  * @param  rx_msg  buffer to hold the message with the position.
  * 
  * @return True if position was got with success, false otherwise. 
  * */
bool get_position(elevador_t* elev, char* rx_msg);

  /**
   * Parse incoming solicitation.
   * 
   * @param elev  elevator with solicitation.
   * @param andar_request floor requested.
   * @param rx_msg message with solicitation.
   * @param exp_msg_floor pointer to string to hold the expected message.
   * */
void parse_solicitation(elevador_t* elev, uint8_t andar_request, char* rx_msg, char* exp_msg_floor);

 /**     
  * Calculates the priority for the current solicitation.   
  * 
  * @param estado current state of the elevator.
  * @param diff_floors difference between the elevator floor and the floor requested.
  * 
  * @return the priority calculated. 
  * */
uint8_t calc_prio(estados estado, int8_t diff_floors);

void send_command(const char* command);

void init_elev(char elev_ch);

void button_ligth_on(char elev_ch, char floor_ch);

void button_ligth_off(char elev_ch, char floor_ch);

void open_door(char elev_ch);

void close_door(char elev_ch);

void stop_elev(char elev_ch);

void go_up(char elev_ch);

void go_down(char elev_ch);
#endif