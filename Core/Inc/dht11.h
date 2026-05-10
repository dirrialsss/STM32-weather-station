/* dht11.h */
#ifndef DHT11_H
#define DHT11_H

#include "stm32f4xx_hal.h"

typedef struct {
    float temperature;
    float humidity;
    uint8_t status;
} DHT11_Data;

// Status codes
#define DHT11_OK 0
#define DHT11_ERROR 1
#define DHT11_TIMEOUT 2

// Function prototypes
void DHT11_Init(GPIO_TypeDef *port, uint16_t pin, TIM_HandleTypeDef *timer);
uint8_t DHT11_Read(DHT11_Data *data);

#endif /* DHT11_H */
