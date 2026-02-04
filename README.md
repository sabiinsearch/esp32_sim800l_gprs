# ESP32 SIM800L GPRS Cloud Messaging

ESP-IDF project for ESP32 to communicate with SIM800L GSM module via UART, establish GPRS connectivity, and send messages to a cloud server.

## Hardware Connections

| SIM800L Pin | ESP32 Pin | Description |
|-------------|-----------|-------------|
| TX          | GPIO2 (D2)| SIM800L transmit to ESP32 receive |
| RX          | GPIO4 (D4)| SIM800L receive from ESP32 transmit |
| GND         | GND       | Common ground |
| VCC         | 4V (2A)   | Power supply (use external power) |

> **Important**: SIM800L requires a stable 4V power supply capable of delivering up to 2A during transmission bursts. Do not power directly from ESP32 3.3V pin.

## Prerequisites

- ESP-IDF v4.4 or later installed
- Active SIM card with data plan
- SIM800L module
- External 4V power supply for SIM800L

## Configuration

Before building, configure your APN and cloud endpoint:

1. Open `main/gprs_client.c`
2. Set your mobile carrier's APN (e.g., "internet", "airtelgprs.com", "www")
3. Set your cloud server URL in `main/gprs_client.h`

## Build and Flash

```bash
# Navigate to project directory
cd esp32_sim800l_gprs

# Build the project
idf.py build

# Flash to ESP32 (replace COM3 with your port)
idf.py -p COM3 flash monitor
```

## Expected Output

```
I (xxx) SIM800L: Initializing SIM800L...
I (xxx) SIM800L: AT command OK
I (xxx) SIM800L: Network registered
I (xxx) SIM800L: GPRS connected
I (xxx) GPRS_CLIENT: Sending SMS to +919876543210...
I (xxx) SIM800L: SMS sent successfully!
I (xxx) GPRS_CLIENT: Sending message to cloud...
I (xxx) GPRS_CLIENT: Message sent successfully
```

## Usage Examples

### Send SMS Message

```c
#include "gprs_client.h"

// Initialize GPRS
gprs_client_init("internet");

// Send SMS to a phone number (use international format)
gprs_client_send_sms("+919876543210", "Hello from ESP32!");
```

### Send HTTP POST to Cloud

```c
// Send data to cloud server
gprs_client_send_message("Sensor data: Temperature=25C");
```

### Complete Example

```c
void app_main(void)
{
    // Initialize
    gprs_client_init("internet");
    
    // Send SMS
    gprs_client_send_sms("+919876543210", "ESP32 started!");
    
    // Send to cloud
    gprs_client_send_message("System online");
    
    // Cleanup
    gprs_client_disconnect();
}
```

## Troubleshooting

- **No response from SIM800L**: Check UART connections and baud rate (9600)
- **Network registration failed**: Verify SIM card is inserted and has signal
- **GPRS connection failed**: Check APN settings for your carrier
- **HTTP request failed**: Verify cloud endpoint URL and network connectivity
- **SMS send failed**: 
  - Ensure phone number is in international format (e.g., "+919876543210")
  - Check if SIM card has SMS credit/balance
  - Verify network registration is complete before sending SMS

## License

MIT
