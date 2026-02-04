#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "gprs_client.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "ESP32 SIM800L GPRS Cloud Messaging");
    ESP_LOGI(TAG, "====================================");

    // Initialize NVS (required for some ESP-IDF components)
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "NVS initialized");

    // Initialize GPRS client
    // NOTE: Change the APN to match your mobile carrier
    // Common APNs:
    //   - "internet" (generic)
    //   - "airtelgprs.com" (Airtel India)
    //   - "www" (Jio India)
    //   - "bsnlnet" (BSNL India)
    //   - "portalnmms" (Vodafone)
    
    const char *apn = "airtelgprs.com";  // CHANGE THIS TO YOUR CARRIER'S APN
    
    ESP_LOGI(TAG, "Initializing GPRS connection with APN: %s", apn);
    ret = gprs_client_init(apn);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPRS initialization failed!");
        ESP_LOGE(TAG, "Please check:");
        ESP_LOGE(TAG, "  1. SIM card is inserted and active");
        ESP_LOGE(TAG, "  2. SIM800L has proper power supply (4V, 2A)");
        ESP_LOGE(TAG, "  3. UART connections (TX->GPIO4, RX->GPIO2)");
        ESP_LOGE(TAG, "  4. APN is correct for your carrier");
        return;
    }

    ESP_LOGI(TAG, "GPRS connected successfully!");

    // ========================================
    // SMS EXAMPLE - Send SMS to a phone number
    // ========================================
    // IMPORTANT: Replace with your actual phone number in international format
    // Example formats:
    //   - "+919876543210" (India)
    //   - "+12025551234" (USA)
    //   - "+447700900123" (UK)
    
    const char *phone_number = "+9650560329";  // CHANGE THIS TO YOUR PHONE NUMBER
    const char *sms_message = "Hello from ESP32! SIM800L is working.";
    
    ESP_LOGI(TAG, "Sending SMS to %s...", phone_number);
    ret = gprs_client_send_sms(phone_number, sms_message);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SMS sent successfully!");
    } else {
        ESP_LOGW(TAG, "Failed to send SMS");
    }

    // ========================================
    // HTTP EXAMPLE - Send message to cloud server
    // ========================================
    ESP_LOGI(TAG, "Sending test message to cloud...");
    ret = gprs_client_send_message("Hello from ESP32! System initialized.");
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Test message sent successfully!");
    } else {
        ESP_LOGW(TAG, "Failed to send test message");
    }

    // Main loop - send periodic updates
    int message_count = 0;
    while (1) {
        // Wait 30 seconds between messages
        vTaskDelay(pdMS_TO_TICKS(30000));

        // Prepare status message
        char status_msg[128];
        snprintf(status_msg, sizeof(status_msg), 
                 "Status update #%d - System running normally", 
                 ++message_count);

        ESP_LOGI(TAG, "Sending periodic update...");
        ret = gprs_client_send_message(status_msg);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Update sent successfully (count: %d)", message_count);
        } else {
            ESP_LOGW(TAG, "Failed to send update");
            
            // If multiple failures, try to reconnect
            if (message_count % 3 == 0) {
                ESP_LOGI(TAG, "Attempting to reconnect...");
                gprs_client_disconnect();
                vTaskDelay(pdMS_TO_TICKS(5000));
                
                ret = gprs_client_init(apn);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Reconnection failed");
                }
            }
        }
    }
}
