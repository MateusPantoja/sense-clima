#include "HT_DHT22.h"
#include "HT_GPIO_Api.h"
#include "bsp.h"           // For pad_config_t, gpio_pin_config_t, delay_us and GPIO_PinRead
#include "task.h"          // For vTaskSuspendAll/xTaskResumeAll

// Timeout values in microseconds for the read loop
#define DHT22_TIMEOUT_RESPONSE_START 80
#define DHT22_TIMEOUT_RESPONSE_PULSE 100
#define DHT22_TIMEOUT_DATA_PULSE     100

void DHT22_Init(void) {
    pad_config_t padConfig;
    gpio_pin_config_t config;

    // Set alternate function to GPIO
    PAD_GetDefaultConfig(&padConfig);
    padConfig.mux = PAD_MuxAlt0;
    PAD_SetPinConfig(DHT22_PAD_ID, &padConfig);

    // Configure pin as input with pull-up.
    // The line is idle high. The read function will switch to output to start communication.
    config.pinDirection = GPIO_DirectionInput;
    GPIO_PinConfig(DHT22_GPIO_INSTANCE, DHT22_GPIO_PIN, &config);
    // NOTE: An external 4.7k pull-up resistor is MANDATORY for DHT22 operation.
    // The internal pull-up is explicitly disabled to ensure reliance on the correct external component.
    PAD_SetPinPullConfig(DHT22_PAD_ID, PAD_AutoPull);
}

int DHT22_Read(float *temperature, float *humidity) {
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint16_t cycles[80]; // Array to store pulse durations
    int ret = 0;
    gpio_pin_config_t config;

    // --- Start of critical section ---
    // Disable task switching to ensure precise timing
    vTaskSuspendAll();
    
    // === STEP 1: Send start signal ===
    // Temporarily configure pin as output
    config.pinDirection = GPIO_DirectionOutput;
    GPIO_PinConfig(DHT22_GPIO_INSTANCE, DHT22_GPIO_PIN, &config);
    
    // Pull line low for > 1ms
    HT_GPIO_WritePin(DHT22_GPIO_PIN, DHT22_GPIO_INSTANCE, 0);
    delay_us(1100); // 1.1ms is a common value
    
    // Pull line high and switch back to input
    HT_GPIO_WritePin(DHT22_GPIO_PIN, DHT22_GPIO_INSTANCE, 1);
    delay_us(40);
    
    config.pinDirection = GPIO_DirectionInput;
    GPIO_PinConfig(DHT22_GPIO_INSTANCE, DHT22_GPIO_PIN, &config);
    
    delay_us(1); // Small delay for pin to settle
    
    // === STEP 2: Wait for sensor response ===
    uint16_t timeout_counter = 0;
    // Wait for pin to go LOW
    while (GPIO_PinRead(DHT22_GPIO_INSTANCE, DHT22_GPIO_PIN)) {
        if (++timeout_counter > DHT22_TIMEOUT_RESPONSE_START) { ret = DHT22_ERROR_TIMEOUT_START; goto end_read; }
        delay_us(1);
    }
    // Wait for pin to go HIGH
    timeout_counter = 0;
    while (!GPIO_PinRead(DHT22_GPIO_INSTANCE, DHT22_GPIO_PIN)) {
        if (++timeout_counter > DHT22_TIMEOUT_RESPONSE_PULSE) { ret = DHT22_ERROR_TIMEOUT_LOW; goto end_read; }
        delay_us(1);
    }
    // Wait for pin to go LOW again (start of data)
    timeout_counter = 0;
    while (GPIO_PinRead(DHT22_GPIO_INSTANCE, DHT22_GPIO_PIN)) {
        if (++timeout_counter > DHT22_TIMEOUT_RESPONSE_PULSE) { ret = DHT22_ERROR_TIMEOUT_HIGH; goto end_read; }
        delay_us(1);
    }
    
    // === STEP 3: Read all 40 bits (80 pulses) into the cycles array ===
    for (int i = 0; i < 80; i += 2) {
        // Measure LOW pulse duration (should be ~50us)
        uint16_t low_duration = 0;
        while (!GPIO_PinRead(DHT22_GPIO_INSTANCE, DHT22_GPIO_PIN)) {
            if (++low_duration > DHT22_TIMEOUT_DATA_PULSE) { ret = DHT22_ERROR_TIMEOUT_DATA; goto end_read; }
            delay_us(1);
        }
        cycles[i] = low_duration;
        
        // Measure HIGH pulse duration (26-28us for 0, 70us for 1)
        uint16_t high_duration = 0;
        while (GPIO_PinRead(DHT22_GPIO_INSTANCE, DHT22_GPIO_PIN)) {
            if (++high_duration > DHT22_TIMEOUT_DATA_PULSE) { ret = DHT22_ERROR_TIMEOUT_DATA; goto end_read; }
            delay_us(1);
        }
        cycles[i + 1] = high_duration;
    }

end_read:
    // --- End of critical section ---
    xTaskResumeAll();
    
    if (ret != 0) {
        return ret; // Return error code
    }
    
    // === STEP 4: Decode pulses from the array into data bytes ===
    for (int i = 0; i < 40; ++i) {
        uint16_t lowCycles = cycles[2 * i];
        uint16_t highCycles = cycles[2 * i + 1];
        
        data[i / 8] <<= 1;
        // The robust way: compare high pulse duration to low pulse duration
        if (highCycles > lowCycles) {
            data[i / 8] |= 1;
        }
    }
    
    // === STEP 5: Verify checksum and calculate final values ===
    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        *humidity = ((uint16_t)(data[0] << 8) | data[1]) / 10.0f;
        uint16_t raw_temp = (uint16_t)(data[2] & 0x7F) << 8 | data[3];
        *temperature = raw_temp / 10.0f;
        if (data[2] & 0x80) { *temperature *= -1.0f; }
        return DHT22_OK; // Success
    }

    return DHT22_ERROR_CHECKSUM; // Checksum error
}