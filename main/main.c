/* The example of ESP-IDF
 *
 * This sample code is in the public domain.
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_flash.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lora.h"

static void get_wake_up_reason();
const int wakeup_time_sec = 60;

// check if target is esp32s3
#ifdef CONFIG_IDF_TARGET_ESP32
	esp_pm_config_esp32_t pm_config;
	esp_pm_config_esp32_t pm_config2;
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S3
	esp_pm_config_esp32s3_t pm_config;
	esp_pm_config_esp32s3_t pm_config2;
#endif


#if CONFIG_SENDER

void task_tx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) {
		TickType_t nowTick = xTaskGetTickCount();
		int send_len = sprintf((char *)buf,"Hello World!! %"PRIu32, nowTick);
		lora_send_packet(buf, send_len);
		ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent...", send_len);
		int lost = lora_packet_lost();
		if (lost != 0) {
			ESP_LOGW(pcTaskGetName(NULL), "%d packets lost", lost);
		}
		vTaskDelay(500/portTICK_PERIOD_MS);
		lora_sleep();
		printf("Going to sleep in 5 second \n");
		vTaskDelay(5000/portTICK_PERIOD_MS);



		#if CONFIG_PM_ENABLE
			pm_config2.max_freq_mhz = 80;
			pm_config2.min_freq_mhz = 10;
			pm_config2.light_sleep_enable = false;
			ESP_ERROR_CHECK( esp_pm_configure(&pm_config2) );
		#endif // CONFIG_PM_ENABLE
		
		printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
		ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
		esp_deep_sleep_start();


		//vTaskDelay(pdMS_TO_TICKS(5000));
	} // end while
}
#endif // CONFIG_SENDER

#if CONFIG_RECEIVER
void task_rx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) {
		lora_receive(); // put into receive mode
		if (lora_received()) {
			int rxLen = lora_receive_packet(buf, sizeof(buf));
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);
		}
		vTaskDelay(1); // Avoid WatchDog alerts
	} // end while
}
#endif // CONFIG_RECEIVER

void app_main()
{
	if (lora_init() == 0) {
		ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module");
		while(1) {
			vTaskDelay(1);
		}
	}

 	esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

	get_wake_up_reason();
	
	#if CONFIG_PM_ENABLE
		pm_config.max_freq_mhz = 80;
		pm_config.min_freq_mhz = 10;
		pm_config.light_sleep_enable = true;
		ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );
	#endif // CONFIG_PM_ENABLE

	#if CONFIG_169MHZ
		ESP_LOGI(pcTaskGetName(NULL), "Frequency is 169MHz");
		lora_set_frequency(169e6); // 169MHz
	#elif CONFIG_433MHZ
		ESP_LOGI(pcTaskGetName(NULL), "Frequency is 433MHz");
		lora_set_frequency(433e6); // 433MHz
	#elif CONFIG_470MHZ
		ESP_LOGI(pcTaskGetName(NULL), "Frequency is 470MHz");
		lora_set_frequency(470e6); // 470MHz
	#elif CONFIG_866MHZ
		ESP_LOGI(pcTaskGetName(NULL), "Frequency is 866MHz");
		lora_set_frequency(866e6); // 866MHz
	#elif CONFIG_915MHZ
		ESP_LOGI(pcTaskGetName(NULL), "Frequency is 915MHz");
		lora_set_frequency(915e6); // 915MHz
	#elif CONFIG_OTHER
		ESP_LOGI(pcTaskGetName(NULL), "Frequency is %dMHz", CONFIG_OTHER_FREQUENCY);
		long frequency = CONFIG_OTHER_FREQUENCY * 1000000;
		lora_set_frequency(frequency);
	#endif

	lora_enable_crc();

	int cr = 1;
	int bw = 7;
	int sf = 7;
	int prea = 10;

#if CONFIF_ADVANCED
	cr = CONFIG_CODING_RATE
	bw = CONFIG_BANDWIDTH;
	sf = CONFIG_SF_RATE;
#endif

	lora_set_coding_rate(cr);
	//lora_set_coding_rate(CONFIG_CODING_RATE);
	//cr = lora_get_coding_rate();
	ESP_LOGI(pcTaskGetName(NULL), "coding_rate=%d", cr);

	lora_set_bandwidth(bw);
	//lora_set_bandwidth(CONFIG_BANDWIDTH);
	//int bw = lora_get_bandwidth();
	ESP_LOGI(pcTaskGetName(NULL), "bandwidth=%d", bw);

	lora_set_spreading_factor(sf);
	//lora_set_spreading_factor(CONFIG_SF_RATE);
	//int sf = lora_get_spreading_factor();
	ESP_LOGI(pcTaskGetName(NULL), "spreading_factor=%d", sf);

	lora_set_preamble_length(prea);
	ESP_LOGI(pcTaskGetName(NULL), "Preamble=%d", prea);

#if CONFIG_SENDER
	xTaskCreate(&task_tx, "TX", 1024*3, NULL, 5, NULL);
#endif
#if CONFIG_RECEIVER
	xTaskCreate(&task_rx, "RX", 1024*3, NULL, 5, NULL);
#endif
}














static void get_wake_up_reason(){
    esp_sleep_source_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch(wakeup_reason)
    {
        case ESP_SLEEP_WAKEUP_EXT0: {
            printf("Wakeup caused by external signal using RTC_IO \n"); 
            break;
        }
        case ESP_SLEEP_WAKEUP_EXT1 : {
            printf("Wakeup caused by external signal using RTC_CNTL \n"); 
            break;
        }
        case ESP_SLEEP_WAKEUP_TIMER : {
            printf("Wakeup caused by timer \n"); 
            break;
        }
        case ESP_SLEEP_WAKEUP_TOUCHPAD : {
            printf("Wakeup caused by touchpad \n"); 
            break;
        }
        case ESP_SLEEP_WAKEUP_ULP : {
            printf("Wakeup caused by ULP program \n"); 
            break;
        }
        default : {
            printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); 
            break;
        }
    }
}