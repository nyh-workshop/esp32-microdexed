#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "driver/timer.h"
#include "math.h"

#include "playtune.h"
#include "songdata.h"

#include "dexed.h"

unsigned char noteState[8];

extern TaskHandle_t nTask2;

Miditones::Miditones() {
  initTimer0();
  timePlay = 0;
  timePlayCount = 0;
  songIndex = 0;
  speed = 1.00f;
  isPlaying = 0;

  xSemaphore = xSemaphoreCreateBinary();
}

void Miditones::updateNote(void* pvParameter) {
  unsigned char cmd = songData1[songIndex];
  unsigned char opcode = cmd & 0xf0;
  unsigned char chan = cmd & 0x0f;
  //unsigned char noteCount = 0;  

  //printf("cmd: %x\n", cmd);

  if (opcode == 0x90) {
    while (opcode == 0x90) {
      // Play a note here! Reset accumulators and set tuning words from the next note!
      // Also reset decay and attack states!

      ((DexedMiditones*)pvParameter)->notePlay(songData1[songIndex + 1], 0xff);

      noteState[chan] = songData1[songIndex + 1];

      songIndex += 2;
      cmd = songData1[songIndex];
      opcode = cmd & 0xf0;
      chan = cmd & 0x0f;
    }

  }

  if (opcode == 0x80) {
    while (opcode == 0x80) {
      // Stop note: the note is dampened immediately in order to prevent the click
      // once the next note is played. Not all the clicks in the sound can be removed: this is currently
      // being investigated.
      
      //noteCount = noteCount & 0x7;
      ((DexedMiditones*)pvParameter)->noteStop(noteState[chan]);  
      //noteCount++;
      
      songIndex += 1;
      cmd = songData1[songIndex];
      opcode = cmd & 0xf0;
      chan = cmd & 0x0f;
      timePlay = 20;
    }
    //unsigned char noteCount = 0;
    return;
  }

  if (opcode == 0xf0) { // stop playing score!
    isPlaying = 0;
    return;
  }

  if (opcode == 0xe0) { // start playing from beginning!
    songIndex = 0;
    timePlay = 0;
    timePlayCount = 0;
    return;
  }

  //cmd = songData1[songIndex];
  //opcode = cmd & 0xf0;

  if ( ((opcode & 0x80) == 0) ||  ((opcode & 0x90) == 0) ) {
    timePlay = (unsigned int)( ((songData1[songIndex] << 8) | songData1[songIndex + 1]) * (1/speed) );
    songIndex += 2;
  }
};

float Miditones::speed = 1.00f;
uint32_t Miditones::timePlay = 0;
uint32_t Miditones::timePlayCount = 0;
uint32_t Miditones::songIndex = 0;
bool Miditones::isPlaying = 0;
SemaphoreHandle_t Miditones::xSemaphore;
QueueHandle_t Miditones::mdtQueue;

void IRAM_ATTR timer_group0_isr(void *para) {
  unsigned char tempVal = 0;
  static BaseType_t xHigherPriorityTaskWoken;
  static int level;

  int timer_idx = (int)para;
  uint32_t intr_status = TIMERG0.int_st_timers.val;
  if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0)
  {
    TIMERG0.hw_timer[timer_idx].update = 1;
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[timer_idx].config.alarm_en = 1;

    if (Miditones::timePlayCount > Miditones::timePlay)
    {
      Miditones::timePlayCount = 0;
      vTaskNotifyGiveFromISR( nTask2, &xHigherPriorityTaskWoken );
    }
    else
      Miditones::timePlayCount++;
  
  }

  portYIELD_FROM_ISR( );
}

// reference: https://github.com/sankarcheppali/esp_idf_esp32_posts/blob/master/timer_group/main/timer_group.c
void Miditones::initTimer0() {
        /* Select and initialize basic parameters of the timer */
    
    const auto TIMER_DIVIDER = 80;
    const auto TIMER_SCALE = (TIMER_BASE_CLK / TIMER_DIVIDER);
    const auto timer_interval_sec = 0.001;
 
    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = 1;
    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    
    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, (uint64_t)(timer_interval_sec * TIMER_SCALE));
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);

    timer_isr_register(TIMER_GROUP_0, TIMER_0, timer_group0_isr, 
        (void *) TIMER_0, ESP_INTR_FLAG_IRAM, NULL);

    timer_start(TIMER_GROUP_0, TIMER_0);

   
};

void updateNote(void* pvParameter, unsigned char& isPlaying, unsigned int& timePlay, unsigned int& timePlayCount, unsigned int& songIndex, float& speed) {

  unsigned char cmd = songData1[songIndex];
  unsigned char opcode = cmd & 0xf0;
  unsigned char chan = cmd & 0x0f;
  //unsigned char noteCount = 0;  

  //printf("cmd: %x\n", cmd);

  if (opcode == 0x90) {
    while (opcode == 0x90) {
      // Play a note here! Reset accumulators and set tuning words from the next note!
      // Also reset decay and attack states!

      ((DexedMiditones*)pvParameter)->notePlay(songData1[songIndex + 1], 0xff);

      noteState[chan] = songData1[songIndex + 1];

      songIndex += 2;
      cmd = songData1[songIndex];
      opcode = cmd & 0xf0;
      chan = cmd & 0x0f;
    }

  }

  if (opcode == 0x80) {
    while (opcode == 0x80) {
      // Stop note: the note is dampened immediately in order to prevent the click
      // once the next note is played. Not all the clicks in the sound can be removed: this is currently
      // being investigated.
      
      //noteCount = noteCount & 0x7;
      ((DexedMiditones*)pvParameter)->noteStop(noteState[chan]);  
      //noteCount++;
      
      songIndex += 1;
      cmd = songData1[songIndex];
      opcode = cmd & 0xf0;
      chan = cmd & 0x0f;
      timePlay = 20;
    }
    //unsigned char noteCount = 0;
    return;
  }

  if (opcode == 0xf0) { // stop playing score!
    isPlaying = 0;
    return;
  }

  if (opcode == 0xe0) { // start playing from beginning!
    songIndex = 0;
    timePlay = 0;
    timePlayCount = 0;
    return;
  }

  //cmd = songData1[songIndex];
  //opcode = cmd & 0xf0;

  if ( ((opcode & 0x80) == 0) ||  ((opcode & 0x90) == 0) ) {
    timePlay = (unsigned int)( ((songData1[songIndex] << 8) | songData1[songIndex + 1]) * (1/speed) );
    songIndex += 2;
  }
}