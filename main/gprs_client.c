#include "gprs_client.h"
#include "sim800l.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "GPRS_CLIENT";
static bool is_connected = false;

esp_err_t gprs_client_init(const char *apn)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing GPRS client...");

    // Initialize SIM800L hardware
    ret = sim800l_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SIM800L");
        return ESP_FAIL;
    }

    // Check network registration
    ret = sim800l_check_network();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Network registration failed");
        return ESP_FAIL;
    }

    // Setup GPRS connection
    const char *apn_to_use = apn ? apn : DEFAULT_APN;
    ret = sim800l_setup_gprs(apn_to_use);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPRS setup failed");
        return ESP_FAIL;
    }

    is_connected = true;
    ESP_LOGI(TAG, "GPRS client initialized successfully");
    return ESP_OK;
}

esp_err_t gprs_client_send_message(const char *message)
{
    if (!is_connected) {
        ESP_LOGE(TAG, "GPRS not connected. Call gprs_client_init() first");
        return ESP_FAIL;
    }

    if (!message) {
        ESP_LOGE(TAG, "Message is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Preparing to send message to cloud...");

    // Format message as JSON
    char json_payload[512];
    snprintf(json_payload, sizeof(json_payload), 
             "{\"device\":\"ESP32\",\"message\":\"%s\",\"timestamp\":%lu}",
             message, (unsigned long)(xTaskGetTickCount() / 1000));

    ESP_LOGI(TAG, "Payload: %s", json_payload);

    // Send HTTP POST request with retry logic
    char response[1024];
    esp_err_t ret = ESP_FAIL;
    int max_retries = 3;

    for (int retry = 0; retry < max_retries; retry++) {
        ret = sim800l_http_post(CLOUD_ENDPOINT_URL, json_payload, response, sizeof(response));
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Message sent successfully!");
            ESP_LOGI(TAG, "Server response: %s", response);
            return ESP_OK;
        }

        ESP_LOGW(TAG, "Send attempt %d/%d failed", retry + 1, max_retries);
        
        if (retry < max_retries - 1) {
            ESP_LOGI(TAG, "Retrying in 2 seconds...");
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    ESP_LOGE(TAG, "Failed to send message after %d attempts", max_retries);
    return ESP_FAIL;
}

esp_err_t gprs_client_send_sms(const char *phone_number, const char *message)
{
    if (!is_connected) {
        ESP_LOGE(TAG, "GPRS not connected. Call gprs_client_init() first");
        return ESP_FAIL;
    }

    if (!phone_number || !message) {
        ESP_LOGE(TAG, "Phone number or message is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending SMS to %s: %s", phone_number, message);
    
    esp_err_t ret = sim800l_send_sms(phone_number, message);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SMS sent successfully!");
    } else {
        ESP_LOGE(TAG, "Failed to send SMS");
    }
    
    return ret;
}

esp_err_t gprs_client_disconnect(void)
{
    if (!is_connected) {
        ESP_LOGW(TAG, "Already disconnected");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Disconnecting GPRS client...");
    
    esp_err_t ret = sim800l_disconnect();
    is_connected = false;
    
    return ret;
}
