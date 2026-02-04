#ifndef SIM800L_H
#define SIM800L_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// UART Configuration
#define SIM800L_UART_NUM        UART_NUM_1
#define SIM800L_TX_PIN          2  // GPIO2 (D2) - ESP32 TX to SIM800L RX (SWAPPED FOR TESTING)
#define SIM800L_RX_PIN          4  // GPIO4 (D4) - ESP32 RX from SIM800L TX (SWAPPED FOR TESTING)
#define SIM800L_BAUD_RATE       9600
#define SIM800L_BUF_SIZE        1024

// Timeout values (milliseconds)
#define SIM800L_CMD_TIMEOUT     5000
#define SIM800L_NETWORK_TIMEOUT 30000
#define SIM800L_HTTP_TIMEOUT    15000

// Response buffer size
#define SIM800L_RESP_BUF_SIZE   2048

/**
 * @brief Initialize SIM800L module and UART communication
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t sim800l_init(void);

/**
 * @brief Send AT command to SIM800L and wait for response
 * 
 * @param cmd AT command string (without \r\n)
 * @param response Buffer to store response
 * @param response_size Size of response buffer
 * @param timeout_ms Timeout in milliseconds
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t sim800l_send_at_cmd(const char *cmd, char *response, size_t response_size, uint32_t timeout_ms);

/**
 * @brief Check if SIM800L is registered on network
 * 
 * @return ESP_OK if registered, ESP_FAIL otherwise
 */
esp_err_t sim800l_check_network(void);

/**
 * @brief Setup GPRS connection with APN
 * 
 * @param apn Access Point Name from mobile carrier
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t sim800l_setup_gprs(const char *apn);

/**
 * @brief Send HTTP POST request
 * 
 * @param url Target URL
 * @param data POST data payload
 * @param response Buffer to store HTTP response
 * @param response_size Size of response buffer
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t sim800l_http_post(const char *url, const char *data, char *response, size_t response_size);

/**
 * @brief Send SMS message to a phone number
 * 
 * @param phone_number Destination phone number (e.g., "+1234567890")
 * @param message Text message to send
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t sim800l_send_sms(const char *phone_number, const char *message);

/**
 * @brief Disconnect GPRS and close HTTP session
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t sim800l_disconnect(void);

#endif // SIM800L_H
