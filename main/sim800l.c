#include "sim800l.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "SIM800L";
static bool is_initialized = false;

esp_err_t sim800l_init(void)
{
    if (is_initialized) {
        ESP_LOGW(TAG, "SIM800L already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing SIM800L on UART%d (TX: GPIO%d, RX: GPIO%d)", 
             SIM800L_UART_NUM, SIM800L_TX_PIN, SIM800L_RX_PIN);

    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = SIM800L_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // Install UART driver
    esp_err_t ret = uart_driver_install(SIM800L_UART_NUM, SIM800L_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_param_config(SIM800L_UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_set_pin(SIM800L_UART_NUM, SIM800L_TX_PIN, SIM800L_RX_PIN, 
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        return ret;
    }

    // Wait for SIM800L to boot up
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Test AT communication
    char response[128];
    ret = sim800l_send_at_cmd("AT", response, sizeof(response), 2000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to communicate with SIM800L");
        return ESP_FAIL;
    }

    // Disable echo
    sim800l_send_at_cmd("ATE0", response, sizeof(response), 2000);

    // Set full functionality
    ESP_LOGI(TAG, "Setting full functionality...");
    sim800l_send_at_cmd("AT+CFUN=1", response, sizeof(response), 3000);

    // Force GSM (2G) mode - common requirement for SIM800L registration
    ESP_LOGI(TAG, "Forcing GSM mode...");
    sim800l_send_at_cmd("AT+CNMP=13", response, sizeof(response), 3000);

    is_initialized = true;
    ESP_LOGI(TAG, "SIM800L initialized successfully");
    return ESP_OK;
}

esp_err_t sim800l_send_at_cmd(const char *cmd, char *response, size_t response_size, uint32_t timeout_ms)
{
    if (!cmd || !response) {
        return ESP_ERR_INVALID_ARG;
    }

    // Clear UART buffer
    uart_flush(SIM800L_UART_NUM);

    // Send command
    char cmd_buf[256];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s\r\n", cmd);
    int len = uart_write_bytes(SIM800L_UART_NUM, cmd_buf, strlen(cmd_buf));
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to send AT command");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Sent: %s", cmd);

    // Wait for response
    memset(response, 0, response_size);
    int total_read = 0;
    uint32_t start_time = xTaskGetTickCount();

    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(timeout_ms)) {
        int available = 0;
        uart_get_buffered_data_len(SIM800L_UART_NUM, (size_t*)&available);
        
        if (available > 0) {
            int read_len = uart_read_bytes(SIM800L_UART_NUM, 
                                          (uint8_t*)(response + total_read),
                                          response_size - total_read - 1,
                                          pdMS_TO_TICKS(100));
            if (read_len > 0) {
                total_read += read_len;
                response[total_read] = '\0';

                // Check for common response endings
                if (strstr(response, "OK\r\n") || strstr(response, "ERROR\r\n") || 
                    strstr(response, "SEND OK\r\n") || strstr(response, ">\r\n")) {
                    break;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGD(TAG, "Response: %s", response);

    // Check if we got a valid response
    if (total_read == 0) {
        ESP_LOGW(TAG, "No response from SIM800L");
        return ESP_ERR_TIMEOUT;
    }

    if (strstr(response, "ERROR")) {
        ESP_LOGW(TAG, "Command returned ERROR");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t sim800l_check_network(void)
{
    char response[256];
    esp_err_t ret;

    ESP_LOGI(TAG, "Checking network registration...");

    // Check SIM card status
    ret = sim800l_send_at_cmd("AT+CPIN?", response, sizeof(response), 3000);
    ESP_LOGI(TAG, "SIM card status: %s", response);
    if (ret != ESP_OK || !strstr(response, "READY")) {
        ESP_LOGE(TAG, "SIM card not ready - check if SIM is inserted properly");
        return ESP_FAIL;
    }

    // Check signal strength
    ret = sim800l_send_at_cmd("AT+CSQ", response, sizeof(response), 3000);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Signal strength: %s", response);
    }

    // Check operator
    ret = sim800l_send_at_cmd("AT+COPS?", response, sizeof(response), 5000);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Operator info: %s", response);
    }

    // Wait for network registration
    for (int i = 0; i < 30; i++) {
        ret = sim800l_send_at_cmd("AT+CREG?", response, sizeof(response), 3000);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "CREG response: %s", response);
            
            // Check for registered (home network or roaming)
            if (strstr(response, "+CREG: 0,1") || strstr(response, "+CREG: 0,5")) {
                ESP_LOGI(TAG, "Network registered successfully");
                return ESP_OK;
            }
            
            // Check for other registration states
            if (strstr(response, "+CREG: 0,2")) {
                ESP_LOGI(TAG, "Searching for network...");
            } else if (strstr(response, "+CREG: 0,3")) {
                ESP_LOGW(TAG, "Registration denied!");
            } else if (strstr(response, "+CREG: 0,0")) {
                ESP_LOGI(TAG, "Not registered, not searching");
            }
        }
        ESP_LOGI(TAG, "Waiting for network registration... (%d/30)", i + 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGE(TAG, "Network registration timeout");
    return ESP_FAIL;
}

esp_err_t sim800l_setup_gprs(const char *apn)
{
    char response[512];
    char cmd[128];
    esp_err_t ret;

    ESP_LOGI(TAG, "Setting up GPRS with APN: %s", apn);

    // Close any existing bearer
    sim800l_send_at_cmd("AT+SAPBR=0,1", response, sizeof(response), 5000);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Set connection type to GPRS
    ret = sim800l_send_at_cmd("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", response, sizeof(response), 3000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set connection type");
        return ESP_FAIL;
    }

    // Set APN
    snprintf(cmd, sizeof(cmd), "AT+SAPBR=3,1,\"APN\",\"%s\"", apn);
    ret = sim800l_send_at_cmd(cmd, response, sizeof(response), 3000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set APN");
        return ESP_FAIL;
    }

    // Open GPRS bearer
    ret = sim800l_send_at_cmd("AT+SAPBR=1,1", response, sizeof(response), 10000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open GPRS bearer");
        return ESP_FAIL;
    }

    // Query bearer status to get IP
    ret = sim800l_send_at_cmd("AT+SAPBR=2,1", response, sizeof(response), 3000);
    if (ret == ESP_OK && strstr(response, "+SAPBR: 1,1")) {
        ESP_LOGI(TAG, "GPRS connected successfully");
        ESP_LOGI(TAG, "Bearer status: %s", response);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "GPRS connection failed");
    return ESP_FAIL;
}

esp_err_t sim800l_http_post(const char *url, const char *data, char *response, size_t response_size)
{
    char cmd[512];
    char resp[SIM800L_RESP_BUF_SIZE];
    esp_err_t ret;

    ESP_LOGI(TAG, "Sending HTTP POST to: %s", url);

    // Initialize HTTP service
    ret = sim800l_send_at_cmd("AT+HTTPINIT", resp, sizeof(resp), 3000);
    if (ret != ESP_OK && !strstr(resp, "ERROR")) {
        ESP_LOGE(TAG, "Failed to initialize HTTP");
        return ESP_FAIL;
    }

    // Set HTTP parameters - CID
    ret = sim800l_send_at_cmd("AT+HTTPPARA=\"CID\",1", resp, sizeof(resp), 3000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set CID");
        sim800l_send_at_cmd("AT+HTTPTERM", resp, sizeof(resp), 2000);
        return ESP_FAIL;
    }

    // Set URL
    snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"URL\",\"%s\"", url);
    ret = sim800l_send_at_cmd(cmd, resp, sizeof(resp), 3000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set URL");
        sim800l_send_at_cmd("AT+HTTPTERM", resp, sizeof(resp), 2000);
        return ESP_FAIL;
    }

    // Set content type
    ret = sim800l_send_at_cmd("AT+HTTPPARA=\"CONTENT\",\"application/json\"", resp, sizeof(resp), 3000);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set content type, continuing...");
    }

    // Set HTTP data
    int data_len = strlen(data);
    snprintf(cmd, sizeof(cmd), "AT+HTTPDATA=%d,10000", data_len);
    ret = sim800l_send_at_cmd(cmd, resp, sizeof(resp), 3000);
    if (ret != ESP_OK || !strstr(resp, "DOWNLOAD")) {
        ESP_LOGE(TAG, "Failed to prepare HTTP data");
        sim800l_send_at_cmd("AT+HTTPTERM", resp, sizeof(resp), 2000);
        return ESP_FAIL;
    }

    // Send actual data
    uart_write_bytes(SIM800L_UART_NUM, data, data_len);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Wait for data acceptance
    memset(resp, 0, sizeof(resp));
    int total_read = 0;
    uint32_t start_time = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(5000)) {
        int read_len = uart_read_bytes(SIM800L_UART_NUM, (uint8_t*)(resp + total_read),
                                      sizeof(resp) - total_read - 1, pdMS_TO_TICKS(100));
        if (read_len > 0) {
            total_read += read_len;
            if (strstr(resp, "OK")) break;
        }
    }

    // Execute HTTP POST
    ret = sim800l_send_at_cmd("AT+HTTPACTION=1", resp, sizeof(resp), SIM800L_HTTP_TIMEOUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute HTTP POST");
        sim800l_send_at_cmd("AT+HTTPTERM", resp, sizeof(resp), 2000);
        return ESP_FAIL;
    }

    // Wait for HTTP response
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Read HTTP response
    ret = sim800l_send_at_cmd("AT+HTTPREAD", resp, sizeof(resp), 10000);
    if (ret == ESP_OK) {
        strncpy(response, resp, response_size - 1);
        response[response_size - 1] = '\0';
        ESP_LOGI(TAG, "HTTP POST successful");
    }

    // Terminate HTTP service
    sim800l_send_at_cmd("AT+HTTPTERM", resp, sizeof(resp), 2000);

    return ret;
}

esp_err_t sim800l_send_sms(const char *phone_number, const char *message)
{
    char cmd[256];
    char resp[SIM800L_RESP_BUF_SIZE];
    esp_err_t ret;

    if (!phone_number || !message) {
        ESP_LOGE(TAG, "Invalid phone number or message");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending SMS to: %s", phone_number);

    // Set SMS text mode
    ret = sim800l_send_at_cmd("AT+CMGF=1", resp, sizeof(resp), 3000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set SMS text mode");
        return ESP_FAIL;
    }

    // Set character set to GSM
    ret = sim800l_send_at_cmd("AT+CSCS=\"GSM\"", resp, sizeof(resp), 3000);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set character set, continuing...");
    }

    // Send SMS command with phone number
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", phone_number);
    ret = sim800l_send_at_cmd(cmd, resp, sizeof(resp), 5000);
    
    // Check for '>' prompt indicating ready to receive message
    if (ret != ESP_OK || !strstr(resp, ">")) {
        ESP_LOGE(TAG, "Failed to initiate SMS send");
        return ESP_FAIL;
    }

    // Send the actual message followed by Ctrl+Z (0x1A)
    uart_write_bytes(SIM800L_UART_NUM, message, strlen(message));
    uart_write_bytes(SIM800L_UART_NUM, "\x1A", 1);

    // Wait for response
    memset(resp, 0, sizeof(resp));
    int total_read = 0;
    uint32_t start_time = xTaskGetTickCount();
    
    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(10000)) {
        int read_len = uart_read_bytes(SIM800L_UART_NUM, (uint8_t*)(resp + total_read),
                                      sizeof(resp) - total_read - 1, pdMS_TO_TICKS(100));
        if (read_len > 0) {
            total_read += read_len;
            resp[total_read] = '\0';
            
            // Check for successful send
            if (strstr(resp, "+CMGS:") || strstr(resp, "OK")) {
                ESP_LOGI(TAG, "SMS sent successfully!");
                ESP_LOGD(TAG, "Response: %s", resp);
                return ESP_OK;
            }
            
            // Check for error
            if (strstr(resp, "ERROR")) {
                ESP_LOGE(TAG, "SMS send failed: %s", resp);
                return ESP_FAIL;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGE(TAG, "SMS send timeout");
    return ESP_ERR_TIMEOUT;
}

esp_err_t sim800l_disconnect(void)
{
    char response[256];
    
    ESP_LOGI(TAG, "Disconnecting GPRS...");
    
    // Close HTTP if active
    sim800l_send_at_cmd("AT+HTTPTERM", response, sizeof(response), 2000);
    
    // Close GPRS bearer
    sim800l_send_at_cmd("AT+SAPBR=0,1", response, sizeof(response), 5000);
    
    ESP_LOGI(TAG, "Disconnected");
    return ESP_OK;
}
