// MicroDexed for ESP32, beta 1.0, by nyh.
// Thanks to Holger W. for the MicroDexed code: https://github.com/dcoredump/MicroDexed
//
// The sample program plays a short tune (Holst's Jupiter) using MicroDexed engine using the I2S module (PCM5102).
//
// Also, thanks to Len Shustek for the Miditones code: https://github.com/LenShustek/miditones
//
// Note: This only works with 8 notes - more than that the whole thing will stutter.
//
// Suggestions are welcome to improve this with more than 8 notes. In the meantime I'm still finding ways and means 
// to improve the FM synth algorithm.
//
// A slight modification is done on dexed.cpp -> the SSAT instruction is replaced by CLAMPS because ESP32 doesn't 
// have SSAT. Fundamentally, CLAMPS works the same as SSAT.
//
// As this is a beta, the code can be very messy. Again, I'm working to reorganize the code as much as possible.
//
// FreeRTOS settings: 1000Hz tick rate.

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "driver/i2s.h"
#include "driver/timer.h"
#include "esp_task_wdt.h"

#include "dexed.h"
#include "config.h"
#include "dx7rom.h"

#include "playtune.h"

#define BUFFER_SIZE 256  // Buffer size in samples!

char voice_name[11];
TaskHandle_t nTask1 = NULL, nTask2 = NULL;
uint32_t overload = 0;

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

int16_t audio_buffer[BUFFER_SIZE];

void microDexedMainTask(void* pvParameter) {
    uint32_t bytesWritten = 0;

    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(5);
    xLastWakeTime = xTaskGetTickCount();

    memset(audio_buffer, 0x00, sizeof(audio_buffer));
    // For debugging purposes! (Measure execution times)
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_16, 0);
    gpio_set_level(GPIO_NUM_17, 0);
    //-------------------------------------------------------
    
    printf("Starting microDexedMainTask...\n");
    printf("microDexedMainTask running on core: %d\n", xPortGetCoreID());

    //xLastWakeTime = xTaskGetTickCount();

    printf("starting time: %llu\n", esp_timer_get_time());

    while (1)
    {
        // This is a very naive method of giving the idle task a chance to do its work!
        // Every 256 samples, or 5.805ms, another 1ms will be given to the idle task.
        for (auto i = 0; i < 256; i++)
        {
            ((DexedMiditones *)pvParameter)->getSamples(BUFFER_SIZE, audio_buffer);
            i2s_write(I2S_NUM_0, audio_buffer, sizeof(audio_buffer), &bytesWritten, portMAX_DELAY);
        }
        vTaskDelay(1);
    }
}

void playTask(void* pvParameter) {
    Miditones mdt;

    printf("Starting playTask...\n");
    printf("playTask running on core: %d\n", xPortGetCoreID());
    
    while (1)
    {
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        mdt.updateNote((DexedMiditones*)pvParameter);
    }
}

extern "C" void app_main(void)
{
    nvs_flash_init();
    //float vol = 1.00f;
    //float vol_left  = 1.00f;
    //float vol_right = 1.00f;

    // b = bank.
    // v = voice number.

    static const i2s_config_t i2s_config = {
         .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
         .sample_rate = 44100,
         .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
         .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
         .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
         .intr_alloc_flags = 0, // default interrupt priority
         .dma_buf_count = 16,
         .dma_buf_len = BUFFER_SIZE*2,
         .use_apll = false,
         .tx_desc_auto_clear = true
    };

    static const i2s_pin_config_t pin_config = {
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_out_num = 22,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);   //install and start i2s driver
    i2s_set_pin(I2S_NUM_0, &pin_config);

    uint8_t v = 7;
    uint8_t voice_number = v;

    // voice data?
    uint8_t data[128];

    //Dexed* dexed = new Dexed(SAMPLE_RATE);
    static DexedMiditones* dxmiditones = new DexedMiditones(SAMPLE_RATE);

    // load directly from the file!
    // we deal directly with one bank first!
    // one bank has 32 voices!
    for (auto n = 0; n < 4096; n++)
    {
        uint8_t d = dx7rom[n + 6];
        if (n >= voice_number * 128 && n < (voice_number + 1) * 128)
        {
            data[n - (voice_number * 128)] = d;
        }
    }

    //dexed->loadSysexVoice(data);
    dxmiditones->loadSysexVoice(data);

    printf("Testing Dexed on ESP32!\n");
    printf("Voice name: %s\n", voice_name);

    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(100);    
 
    xTaskCreate(&microDexedMainTask, "microDexedMainTask", 4096, dxmiditones, 2, &nTask1);
    xTaskCreatePinnedToCore(&playTask,"playTask",2048,dxmiditones, 1, &nTask2, 0 );
    
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    int level = 0;
    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
