#ifndef GPRS_CLIENT_H
#define GPRS_CLIENT_H

#include "esp_err.h"

// Cloud configuration - UPDATE THESE VALUES
#define CLOUD_ENDPOINT_URL  "http://your-server.com/api/data"  // Change to your cloud URL
#define DEFAULT_APN         "internet"  // Change to your carrier's APN

/**
 * @brief Initialize GPRS client and establish connection
 * 
 * @param apn Access Point Name (NULL to use default)
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t gprs_client_init(const char *apn);

/**
 * @brief Initiate a voice call to a phone number
 * 
 * @param phone_number Destination phone number
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t gprs_client_make_call(const char *phone_number);

/**
 * @brief Connect to GPRS network (Internet)
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t gprs_client_connect_gprs(void);

/**
 * @brief Send message to cloud server
 * 
 * @param message Message payload (will be sent as JSON)
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t gprs_client_send_message(const char *message);

/**
 * @brief Send SMS to a phone number
 * 
 * @param phone_number Destination phone number (e.g., "+1234567890")
 * @param message Text message to send
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t gprs_client_send_sms(const char *phone_number, const char *message);

/**
 * @brief Disconnect from GPRS network
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t gprs_client_disconnect(void);

#endif // GPRS_CLIENT_H
