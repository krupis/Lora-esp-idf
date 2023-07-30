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


#define GPIO_INPUT_IO_0 11
#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_INPUT_IO_0))

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void *arg)
{
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            printf("GPIO[%" PRIu32 "] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

static void configure_dio0_interrupt_pin();

static void task_tx(void *p);
void task_rx(void *p);

static void task_tx(void *p)
{
    printf("create lora tx task \n");
    for (;;)
    {
        lora_send_packet((uint8_t *)"Hello", 5);
        printf("packet sent...\n");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}



void task_rx(void *p)
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
    configure_dio0_interrupt_pin();

    printf("Initializing lora \n");
    lora_init();
    printf("Setting frequency \n");
    lora_set_frequency(433e6);
    printf("Enablig crc \n");

    int dio0_map = lora_read_reg(0x40);
    printf("dio0 map read \n");
    printf("Reading DIO map register (0x40) = %.2x \n", dio0_map);

    printf("writing 0x01 to dio0 map\n");
    lora_write_reg(0x40, 0b01000000);
    int dio0_map_after_write = lora_read_reg(0x40);
    printf("dio0 map read after write \n");
    printf("Reading DIO map register after write (0x40) = %.2x \n", dio0_map_after_write);

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

static void configure_dio0_interrupt_pin()
{
    gpio_config_t io_conf;
    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    // bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_down_en = 1;
    // io_conf.pull_up_en = 0;
    gpio_config(&io_conf);


    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    // install gpio isr service
    gpio_install_isr_service(0);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void *)GPIO_INPUT_IO_0);
}