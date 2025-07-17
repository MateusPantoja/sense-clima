#ifndef __HT_DHT22_H__
#define __HT_DHT22_H__

#include <stdint.h>
#include "bsp.h" // For pad_config_t, gpio_pin_config_t

#define DHT22_GPIO_INSTANCE 0
#define DHT22_GPIO_PIN      2
// According to the GPIO Table in HT_GPIO_Api.h, GPIO2 is on PAD ID 13.
#define DHT22_PAD_ID        13 //2 // o Correta para uso Ã© o Pin Number da GPIO0_2

// DHT22_Read function return codes
#define DHT22_OK                    0   // Success
#define DHT22_ERROR_TIMEOUT_START   -1  // Timeout waiting for sensor response to start
#define DHT22_ERROR_TIMEOUT_LOW     -2  // Timeout waiting for response low pulse to end
#define DHT22_ERROR_TIMEOUT_HIGH    -3  // Timeout waiting for response high pulse to end
#define DHT22_ERROR_TIMEOUT_DATA    -4  // Timeout during data bit reception
#define DHT22_ERROR_CHECKSUM        -5  // Checksum mismatch

// Initializes the GPIO pin for the DHT22 sensor.
void DHT22_Init(void);

// Reads temperature and humidity from the DHT22 sensor.
int DHT22_Read(float *temperature, float *humidity);

#endif // __HT_DHT22_H__ 