/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "lora.h"
#include "nvs_flash.h"

#include "freertos/queue.h"
#include "driver/gpio.h"

#define LORA_MODE   0 // 0-receive/ 1-transmit

uint8_t buf[32];



static void task_tx(void *p);
static void task_rx(void *p);





static void task_tx(void *p)
{
    printf("create lora tx task \n");
    for (;;)
    {
        lora_send_packet((uint8_t *)"Hello", 6);
        printf("packet sent...\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}



static void task_rx(void *p)
{
   int x;
   for(;;) {
      lora_receive();    // put into receive mode
      while(lora_received()) {
         x = lora_receive_packet(buf, sizeof(buf));
         buf[x] = 0;
         printf("Received: %s\n", buf);
         lora_receive();
      }
      vTaskDelay(1);
   }
}


void app_main()
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if(LORA_MODE == 1){
        printf("starting lora transmit mode \n");
        //configure_dio0_interrupt_pin();
        lora_init();
        lora_set_frequency(433e6);
        lora_enable_crc();
        xTaskCreate(&task_tx, "task_tx", 2048, NULL, 5, NULL);
    }
    else if(LORA_MODE == 0){
        printf("starting lora receive mode \n");
        lora_init();
        lora_set_frequency(433e6);
        lora_enable_crc();
        xTaskCreate(&task_rx, "task_rx", 2048, NULL, 5, NULL);
    }
}
