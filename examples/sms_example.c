/**
 * @file sms_example.c
 * @brief Simple example showing how to send SMS using SIM800L
 * 
 * This is a standalone example showing the minimal code needed
 * to initialize the SIM800L and send an SMS message.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "gprs_client.h"

static const char *TAG = "SMS_EXAMPLE";

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "ESP32 SIM800L SMS Example");
    ESP_LOGI(TAG, "==========================");

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // ========================================
    // STEP 1: Initialize GPRS with your APN
    // ========================================
    // Common APNs in India:
    //   - "internet" (generic)
    //   - "airtelgprs.com" (Airtel)
    //   - "www" (Jio)
    //   - "bsnlnet" (BSNL)
    
    const char *apn = "internet";  // CHANGE THIS
    
    ESP_LOGI(TAG, "Initializing SIM800L with APN: %s", apn);
    ret = gprs_client_init(apn);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SIM800L!");
        ESP_LOGE(TAG, "Check hardware connections and SIM card");
        return;
    }

    ESP_LOGI(TAG, "SIM800L initialized successfully!");

    // ========================================
    // STEP 2: Send SMS
    // ========================================
    // IMPORTANT: Use international format for phone number
    // Format: +[country code][number]
    // Examples:
    //   India: +919876543210
    //   USA:   +12025551234
    //   UK:    +447700900123
    
    const char *phone_number = "+919876543210";  // CHANGE THIS TO YOUR NUMBER
    const char *message = "Hello! This is a test message from ESP32 with SIM800L.";
    
    ESP_LOGI(TAG, "Sending SMS to: %s", phone_number);
    ESP_LOGI(TAG, "Message: %s", message);
    
    ret = gprs_client_send_sms(phone_number, message);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ SMS sent successfully!");
    } else {
        ESP_LOGE(TAG, "✗ Failed to send SMS");
        ESP_LOGE(TAG, "Check:");
        ESP_LOGE(TAG, "  - Phone number format (+country code + number)");
        ESP_LOGE(TAG, "  - SIM card has SMS balance");
        ESP_LOGE(TAG, "  - Network signal strength");
    }

    // ========================================
    // STEP 3: Send another SMS (optional)
    // ========================================
    ESP_LOGI(TAG, "Waiting 5 seconds before sending another SMS...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    ret = gprs_client_send_sms(phone_number, "Second test message from ESP32!");
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Second SMS sent successfully!");
    } else {
        ESP_LOGE(TAG, "✗ Failed to send second SMS");
    }

    // ========================================
    // STEP 4: Cleanup
    // ========================================
    ESP_LOGI(TAG, "Disconnecting...");
    gprs_client_disconnect();
    
    ESP_LOGI(TAG, "Example complete!");
}
