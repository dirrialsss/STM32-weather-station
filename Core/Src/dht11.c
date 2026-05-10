/* dht11.c */
#include "dht11.h"

static GPIO_TypeDef *DHT11_PORT;
static uint16_t DHT11_PIN;
static TIM_HandleTypeDef *DHT11_TIMER;

// Microsecond delay function
static void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(DHT11_TIMER, 0);
    while(__HAL_TIM_GET_COUNTER(DHT11_TIMER) < us);
}

// Set pin as output
static void DHT11_SetPinOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

// Set pin as input
static void DHT11_SetPinInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

void DHT11_Init(GPIO_TypeDef *port, uint16_t pin, TIM_HandleTypeDef *timer) {
    DHT11_PORT = port;
    DHT11_PIN = pin;
    DHT11_TIMER = timer;

    // Start timer
    HAL_TIM_Base_Start(DHT11_TIMER);
}

uint8_t DHT11_Read(DHT11_Data *data) {
    uint8_t buffer[5] = {0};
    uint8_t i, j;
    uint16_t timeout;

    // Send start signal
    DHT11_SetPinOutput();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(18); // At least 18ms

    DHT11_SetPinInput();
    delay_us(30);

    // Check response
    if(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
        return DHT11_ERROR;
    }

    // Wait for response to go high
    timeout = 0;
    while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET) {
        delay_us(1);
        if(++timeout > 100) return DHT11_TIMEOUT;
    }

    // Wait for response to go low
    timeout = 0;
    while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
        delay_us(1);
        if(++timeout > 100) return DHT11_TIMEOUT;
    }

    // Read 40 bits (5 bytes)
    for(j = 0; j < 5; j++) {
        for(i = 0; i < 8; i++) {
            // Wait for signal to go high
            timeout = 0;
            while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET) {
                delay_us(1);
                if(++timeout > 100) return DHT11_TIMEOUT;
            }

            // Measure high time
            delay_us(40);

            if(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
                buffer[j] |= (1 << (7 - i));
            }

            // Wait for signal to go low
            timeout = 0;
            while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
                delay_us(1);
                if(++timeout > 100) return DHT11_TIMEOUT;
            }
        }
    }

    // Verify checksum
    if(buffer[4] != ((buffer[0] + buffer[1] + buffer[2] + buffer[3]) & 0xFF)) {
        return DHT11_ERROR;
    }

    // Parse data
    data->humidity = (float)buffer[0];
    data->temperature = (float)buffer[2];
    data->status = DHT11_OK;

    return DHT11_OK;
}
