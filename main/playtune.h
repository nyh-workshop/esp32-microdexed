#ifndef PLAYTUNE_H
#define PLAYTUNE_H

#include "dexed.h"
#include "freertos/FreeRTOS.h"

// I prefer to leave the library untouched!
class DexedMiditones : public Dexed {
    public:
    DexedMiditones(int rate) : Dexed(rate) {};
    void notePlay(uint8_t note, uint8_t velocity) { keydown(note, velocity); }
    void noteStop(uint8_t note) { keyup(note); }
};

class Miditones {
    public:
    Miditones();
    void updateNote(void* pvParameter);
    inline SemaphoreHandle_t getSemaphore() { return xSemaphore; }
    inline QueueHandle_t getQueue() { return mdtQueue; }
    //static void IRAM_ATTR timer_group0_isr(void *para);

    private:
    void initTimer0();    
    static unsigned int timePlay;
    static unsigned int timePlayCount;
    static unsigned int songIndex;
    static float speed;
    static bool isPlaying;
    
    friend void IRAM_ATTR timer_group0_isr(void *para);

    static SemaphoreHandle_t xSemaphore;
    static QueueHandle_t mdtQueue;
};

void updateNote(void* pvParameter, unsigned char& isPlaying, unsigned int& timePlay, unsigned int& timePlayCount, unsigned int& songIndex, float& speed);

#endif