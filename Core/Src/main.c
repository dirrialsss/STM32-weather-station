/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Weather Station Main Program
  * @author         : Diana Siukalo
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>                    // for sprintf, printf
#include <string.h>                   // for strlen
#include "LiquidCrystal_I2C.h"        // I2C LCD driver
#include "dht11.h"                    // DHT11 sensor driver
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/**
 * @brief System states based on temperature reading
 */
typedef enum {
  STATE_NORMAL,
  STATE_WARNING,
  STATE_CRITICAL
} SystemState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
SystemState currentState = STATE_NORMAL;    // Holds the current system state
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 * @brief Retarget printf to UART2 – sends characters via USART2
 * @param ch Character to send
 * @return The character sent (standard for putchar)
 */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

// RGB LED pin definitions on Port D  (for KY-016)
#define LED_R_PIN   GPIO_PIN_14    // Red LED control pin
#define LED_G_PIN   GPIO_PIN_12    // Green LED control pin
#define LED_B_PIN   GPIO_PIN_15    // Blue LED control pin
#define LED_PORT    GPIOD          // GPIO port for all RGB LEDs
/**
 * @brief Set the RGB LED to a specific color by controlling each pin
 * @param r 1 = Red ON, 0 = OFF
 * @param g 1 = Green ON, 0 = OFF
 * @param b 1 = Blue ON, 0 = OFF
 */
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
    HAL_GPIO_WritePin(LED_PORT, LED_R_PIN, r ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED_G_PIN, g ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED_B_PIN, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
/**
 * @brief Turn on only the red LED
 */
void setRed()     { setRGB(1,0,0); }
/**
 * @brief Turn on only the green LED
 */
void setGreen()   { setRGB(0,1,0); }
/**
 * @brief Turn on yellow (red + green)
 */
void setYellow()  { setRGB(1, 1, 0); }


/**
 * @brief Generate a tone on the buzzer using PWM on TIM3 channel 2
 * @param freq Desired frequency in Hz. Pass 0 to stop the buzzer.
 */
void Buzzer_PlayTone(uint16_t freq) {
    if (freq == 0) {
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);  // Turn off PWM, silence buzzer
        return;
    }

    uint32_t arr = 1000000 / freq;
    if (arr > 65535) arr = 65535;
    if (arr < 2) arr = 2;


    __HAL_TIM_SET_AUTORELOAD(&htim3, arr - 1);                          // Set PWM period
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, (arr - 1) / 2);

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);                          // Start PWM output
}

/**
 * @brief Update the system state based on the measured temperature
 * @param temperature Current temperature in degrees Celsius
 */
void updateState(float temperature) {
    SystemState newState = currentState;
    if (temperature > 30.0f) {
        newState = STATE_CRITICAL;
    } else if (temperature > 25.0f) {
        newState = STATE_WARNING;
    } else {
        newState = STATE_NORMAL;
    }
    if (newState != currentState) {
        currentState = newState;       // State changed, update variable

    }
}

/**
 * @brief Perform actions according to the current state: set LED color and buzzer.
 */
void applyState(void) {
	switch (currentState) {
	case STATE_NORMAL:
		setGreen();                 // Green LED for normal condition
		Buzzer_PlayTone(0);        // No buzzer sound
		break;
	case STATE_WARNING:
		setYellow();               // Yellow LED as warning
		Buzzer_PlayTone(2000);     // One short beep (2000 Hz for 200 ms)
		HAL_Delay(200);
		Buzzer_PlayTone(0);
		break;
	case STATE_CRITICAL:
		setRed();                  // Red LED for critical high temperature
		for (int i = 0; i < 3; i++) {   // Three short beeps
			Buzzer_PlayTone(2500);
			HAL_Delay(200);
			Buzzer_PlayTone(0);
			HAL_Delay(200);
		}
		break;
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
	HAL_TIM_Base_Start(&htim2);  // Start timer2 used by DHT11 for precise microsecond delays
	HD44780_Init(2);             // Initialize LCD (16x2 characters)
	HD44780_Clear();
	HD44780_SetCursor(0, 0);
	HD44780_PrintStr("Weather Station");
	HAL_Delay(2000);
	HD44780_Clear();
	HD44780_SetCursor(1, 0);
	HD44780_PrintStr("Loading...");
	HAL_Delay(2000);
	DHT11_Init(GPIOA, GPIO_PIN_1, &htim2); // Initialize DHT11 sensor on pin PA1
	HD44780_Clear();
	printf("Weather station started\r\n");   // Send a welcome message over UART
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
		DHT11_Data sensor_data;
		uint8_t read_status = DHT11_Read(&sensor_data);   // Read from sensor

		if (read_status == DHT11_OK) {
			updateState(sensor_data.temperature);    //Update state machine based on measured temperature

			//Display temperature and humidity on the LCD
			char lcd_line1[17];
			char lcd_line2[17];
			sprintf(lcd_line1, "Temp: %.1f C", sensor_data.temperature);
			sprintf(lcd_line2, "Hum:  %.1f %%", sensor_data.humidity);
			HD44780_SetCursor(0, 0);
			HD44780_PrintStr(lcd_line1);
			HD44780_SetCursor(0, 1);
			HD44780_PrintStr(lcd_line2);

			applyState(); //Apply the state (LED + buzzer)

			//Print the data and state to UART for monitoring
			const char *state_str = "";
			switch (currentState) {
			case STATE_NORMAL:
				state_str = "NORMAL";
				break;
			case STATE_WARNING:
				state_str = "WARNING";
				break;
			case STATE_CRITICAL:
				state_str = "CRITICAL";
				break;
			default:
				state_str = "UNKNOWN";
				break;
			}
			printf("Temperature=%.1f C, Humidity=%.1f %% | State: %s\r\n",
					sensor_data.temperature, sensor_data.humidity, state_str);
		} else {
			// Sensor read error
			HD44780_Clear();
			// Three error beeps
			for (int i = 0; i < 3; i++) {
				Buzzer_PlayTone(2000);
				HAL_Delay(200);
				Buzzer_PlayTone(0);
				HAL_Delay(200);
			}
			// Display error message on first line of LCD
			HD44780_SetCursor(0, 0);
			HD44780_PrintStr("DHT11 Error!");
			 // Show error code on second line
			HD44780_SetCursor(0, 1);
			char err_buf[17];
			sprintf(err_buf, "Code: %d", read_status);
			HD44780_PrintStr(err_buf);
			// Send error message over UART
			char err_msg[32];
			sprintf(err_msg, "DHT11 Error: %d\r\n", read_status);
			HAL_UART_Transmit(&huart2, (uint8_t*) err_msg, strlen(err_msg),
					100);
		}
		// Wait 2 seconds before next  cycle
		HAL_Delay(2000);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 15;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 84-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pins : PD12 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
