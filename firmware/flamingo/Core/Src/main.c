/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

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

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
UART_HandleTypeDef huart2;
uint8_t uartTest[5] = "Peep!";

I2C_HandleTypeDef hi2c1;
// Register values
uint8_t LD_ADDR = 0b00011101 << 1;
const uint8_t reg_whoAmI = 0x0F;
const uint8_t reg_offset_x = 0x16;
const uint8_t reg_offset_y = 0x17;
const uint8_t reg_offset_z = 0x18;
const uint8_t reg_ctrl_reg1 = 0x20;
const uint8_t reg_ctrl_reg2 = 0x21;
const uint8_t reg_ctrl_reg3 = 0x22;
const uint8_t reg_outX_l = 0x28;
const uint8_t reg_outX_h = 0x29;
const uint8_t reg_outY_l = 0x2A;
const uint8_t reg_outY_h = 0x2B;
const uint8_t reg_outZ_l = 0x2C;
const uint8_t reg_outZ_h = 0x2D;

const uint16_t x_offset = 500;
const uint16_t z_offset = 280;

//const uint16_t pid_target = 1024;
const uint16_t pid_target = 1050;
const uint16_t pid_target_range = 100;
int16_t pid_error;
float pid_kp = 0.92;
float pid_ki = 0.0000;
float pid_kd = 0.39;
float pid_proportional = 0;
float pid_integral = 0;
float pid_derivative = 0;
float pid_combinedError = 0;
float pid_lastError = 0;
float pid_totalError = 0;
float pid_dc_offset = 15;

uint16_t motarDuty = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void isDeviceAlive(uint8_t);
void initLDDevice(void);
void initMotors();
void i2cWriteOneByte(uint8_t, uint8_t, uint8_t);
void setMotorDirectionForwards();
void setMotorDirectionBackwards();
uint8_t i2cReadOneByte(uint8_t, uint8_t);
uint16_t getJustifiedData(uint8_t, uint8_t);
uint16_t twosComplement(uint16_t);
uint8_t checkDirection(uint16_t);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint8_t buf[12];

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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  // Start of program
  // Configure device so its ready to print data
  initLDDevice();

  // Check if i2c device(s) are alive
  isDeviceAlive(LD_ADDR);

  // Initialises motars
  initMotors();

  // Tests each motor for their speed and direction
  // testMotors();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // Blink LED
	  HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_0);

	  // Gets acceleration data from LD device
	  uint16_t outX = getJustifiedData(reg_outX_l, reg_outX_h);
	  float gX = twosComplement(outX) - x_offset;

	  uint16_t outZ = getJustifiedData(reg_outZ_l, reg_outZ_h) + z_offset;
	  uint8_t direction = checkDirection(outZ);
	  float gZ = outZ;



	  // --- PID Control code ---
	  // Calculate the error between the desired target
	  pid_error = (pid_target - gX)/1;

	  if(pid_target-pid_target_range < pid_target < pid_target+pid_target_range)
	  {
		  // Set direction of motors to be correct
		  	  if(direction == 1)
		  	  {
		  		  setMotorDirectionForwards();
		  	  }
		  	  else
		  	  {
		  		  setMotorDirectionBackwards();
		  	  }

	  // Calculate the integral error
	  if(pid_error != 0)
	  {
		  pid_totalError += pid_error;
	  }
	  else
	  {
		  pid_totalError = 0;
	  }

	  // Calculate all errors in PID controller
	  pid_proportional = pid_error * pid_kp;
	  pid_integral = pid_totalError * pid_ki;
	  pid_derivative = (pid_error - pid_lastError) * pid_kd;

	  pid_lastError = pid_error;

	  pid_combinedError = pid_proportional + pid_integral + pid_derivative;



	  // Flips sign if total error is negative
	  if(pid_combinedError < 0)
	  {
		  pid_combinedError *= -1;
	  }
	  if(pid_combinedError >= 256)
	  {
		  pid_combinedError = 256;
	  }

	  // Due to accelerometer calibration errors, a DC bias is added to the control loop when flamingo is tilted forwards
	  if(direction == 1)
	  {
		pid_combinedError += pid_dc_offset;
	  }

	  // Set the motor drivers duty
	  motarDuty = pid_combinedError;
	  TIM2->CCR1 = motarDuty+pid_dc_offset;
	  TIM2->CCR2 = motarDuty+pid_dc_offset;

	  }
	  else
	  {
		  TIM2->CCR1 = 0;
		  TIM2->CCR2 = 0;
	  }

	  // --- End of PID Control code ---

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
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
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 10;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 256;
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
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  huart2.Init.BaudRate = 38400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_MultiProcessor_Init(&huart2, 0, UART_WAKEUPMETHOD_IDLELINE) != HAL_OK)
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : PF0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PA3 PA4 PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
// User Functions

// Blinks LED quickly 3 times
void ledSuccess()
{
	for(int i = 0; i < 6; i ++)
	{
		HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_0);
		HAL_Delay(50);
	}
}

// Checks if device is alive, if so it blinks LED fast 5 times
void isDeviceAlive(uint8_t slaveAddress)
{
	if(HAL_I2C_IsDeviceReady(&hi2c1, LD_ADDR, 5, 100) == HAL_OK)
	{
		ledSuccess();
	}
}

// Writes one byte to the a register
void i2cWriteOneByte(uint8_t slaveAddress, uint8_t regAddress, uint8_t data)
{
	uint8_t buf[2];

	buf[0] = 0x20;
	buf[1] = 0xCF;
  	HAL_I2C_Master_Transmit(&hi2c1, LD_ADDR, buf, 2, 100);
}

// Reads one byte on slave device
uint8_t i2cReadOneByte(uint8_t slaveAddress, uint8_t regAddress)
{
	uint8_t buf[1];
	uint8_t buf2[1];
	buf[0] = regAddress;
	HAL_I2C_Master_Transmit(&hi2c1, slaveAddress, buf, 1, 100);

	HAL_I2C_Master_Receive(&hi2c1, LD_ADDR, buf2, 1, 100);
	return buf2[0];
}

// Combines low and high bytes to get the justified data
uint16_t getJustifiedData(uint8_t LSBAddress, uint8_t HSBAddress)
{
	int16_t justifiedData;
	uint8_t lsbData = i2cReadOneByte(LD_ADDR, LSBAddress);
	uint8_t hsbData = i2cReadOneByte(LD_ADDR, HSBAddress);

	justifiedData = ((lsbData | ((int16_t)hsbData << 8 )) & 0x0FFF);

	// Compute twos complement
	if(justifiedData > 0x7FF)
	{
		justifiedData |= 0xF000;
		// Gets magnitude for twos complement
		// justifiedData = ((~justifiedData) + 1) & 0x0FFF;
	}

	return(justifiedData);
}

// Returns the twos complement of a 12 bit number
uint16_t twosComplement(uint16_t data)
{
	//if(data > 0x07FF)
	if(data > 0x07FF)
	{
		data = (((~data) + 1) & 0x0FFF);
	}
	return(data);
}

// Checks the direction of which way the board is tilting
// If it returns 0, it if tilting backwards
// If it returns 1, it is tilting forwards
uint8_t checkDirection(uint16_t data)
{
	uint8_t direction = 0;

	if(data > 0x07FF)
	{
		direction = 1;
	}

	return(direction);
}

// Inits LD device
void initLDDevice(void)
{
	// Set CS high
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 1);
	// Turn on device for measurements
	i2cWriteOneByte(LD_ADDR, 0x20, 0xCF);

	// Reads the who am I register and outputs it on UART
	uint8_t data = i2cReadOneByte(LD_ADDR, reg_whoAmI);
	HAL_UART_Transmit(&huart2, &data, 1, 100);
}

// Start PWM channels on DRV devices
void initMotors()
{
	// Turn on PWM
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

	// Enable devices
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, 1);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
}

// Tests motars on their speed and direction
// Note:	Forward direction: pin4=1, pin7=0
//			Backward direction: pin4=0, pin7=1
void testMotors()
{
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7);

	for(int j = 0; j < 2; j ++)
	{
		for(int i = 0; i < 256; i ++)
		{
			if(i > 00)
				TIM2->CCR1 = i+00;
				TIM2->CCR2 = i+00;
			HAL_Delay(10);
		}
		for(int i = 256; i > 5; i --)
		{
			TIM2->CCR1 = i+00;
			TIM2->CCR2 = i+00;
			HAL_Delay(10);
		}
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4);
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7);
	}
}

// Sets the motor direction forwards
void setMotorDirectionForwards()
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, 1);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 0);
}

// Sets the motor direction backwards
void setMotorDirectionBackwards()
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, 0);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 1);
}

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

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
