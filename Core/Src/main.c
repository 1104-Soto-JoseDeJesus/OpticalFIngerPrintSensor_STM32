/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "string.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FP_START_CODE          0xEF01
#define FP_DEFAULT_ADDRESS     0xFFFFFFFF
#define FP_PACKET_COMMAND      0x01
#define FP_PACKET_ACK          0x07
#define FP_TIMEOUT_MS          1000

#define FP_OK                  0x00
#define FP_NO_FINGER           0x02
#define FP_IMAGEFAIL           0x03
#define FP_IMAGEMESS           0x06
#define FP_FEATUREFAIL         0x07
#define FP_NOMATCH             0x09

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma location=0x2007c000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x2007c0a0
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __CC_ARM )  /* MDK ARM Compiler */

__attribute__((at(0x2007c000))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((at(0x2007c0a0))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __GNUC__ ) /* GNU Compiler */

ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".RxDecripSection"))); /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".TxDecripSection")));   /* Ethernet Tx DMA Descriptors */
#endif

ETH_TxPacketConfig TxConfig;

ETH_HandleTypeDef heth;

UART_HandleTypeDef huart6;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* USER CODE BEGIN PV */
typedef struct
{
  uint16_t page_id;
  const char *label;
} FingerEntry_t;

static const FingerEntry_t kFingerDatabase[] = {
    {1, "Member 1 - Right Thumb"},
    {2, "Member 1 - Right Index"},
    {3, "Member 1 - Right Middle"},
    {4, "Member 1 - Left Thumb"},
    {5, "Member 1 - Left Index"},
    {6, "Member 1 - Left Middle"},
    {7, "Member 2 - Right Thumb"},
    {8, "Member 2 - Right Index"},
    {9, "Member 2 - Right Middle"},
    {10, "Member 2 - Left Thumb"},
    {11, "Member 2 - Left Index"},
    {12, "Member 2 - Left Middle"},
    {13, "Member 3 - Right Thumb"},
    {14, "Member 3 - Right Index"},
    {15, "Member 3 - Right Middle"},
    {16, "Member 3 - Left Thumb"},
    {17, "Member 3 - Left Index"},
    {18, "Member 3 - Left Middle"},
    {19, "Member 4 - Right Thumb"},
    {20, "Member 4 - Right Index"},
    {21, "Member 4 - Right Middle"},
    {22, "Member 4 - Left Thumb"},
    {23, "Member 4 - Left Index"},
    {24, "Member 4 - Left Middle"},
};

static uint8_t fp_tx_buffer[32];
static uint8_t fp_rx_buffer[32];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ETH_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
/* USER CODE BEGIN PFP */
static void Fingerprint_Announce(const char *message);
static HAL_StatusTypeDef Fingerprint_VerifyPassword(void);
static HAL_StatusTypeDef Fingerprint_Enroll(uint16_t page_id);
static HAL_StatusTypeDef Fingerprint_CaptureAndSearch(uint16_t *page_id);
static void Fingerprint_ReportMatch(uint16_t page_id);
static void Fingerprint_HandleNoMatch(void);
static void Fingerprint_EnrollDatabase(void);
static void Fingerprint_PromptStartupEnrollment(void);

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
  MX_ETH_Init();
  MX_USART6_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  /* USER CODE BEGIN 2 */

  /* Give the fingerprint sensor time to power up before issuing commands. */
  HAL_Delay(800);

  Fingerprint_Announce("\r\n---Group 11 is da Best---\r\n");

  HAL_StatusTypeDef sensor_status = HAL_ERROR;
  for (int attempt = 0; attempt < 3; ++attempt)
  {
    sensor_status = Fingerprint_VerifyPassword();
    if (sensor_status == HAL_OK)
    {
      break;
    }

    if (sensor_status == HAL_TIMEOUT)
    {
      Fingerprint_Announce("Sensor not ready yet (timeout). Retrying...\r\n");
    }
    else
    {
      Fingerprint_Announce("Sensor communication failed. Retrying...\r\n");
    }

    HAL_Delay(300);
  }

  if (sensor_status != HAL_OK)
  {
    Fingerprint_Announce("Sensor communication failed. Check wiring and power.\r\n");
    Error_Handler();
  }

  Fingerprint_PromptStartupEnrollment();

  Fingerprint_Announce("Sensor ready. Present a registered finger...\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  GPIO_PinState prev_button_state = HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin);
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    GPIO_PinState button_state = HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin);
    if ((button_state == GPIO_PIN_SET) && (prev_button_state == GPIO_PIN_RESET))
    {
      Fingerprint_Announce("Enrollment mode requested. Capturing templates for all configured users...\r\n");
      Fingerprint_EnrollDatabase();
      Fingerprint_Announce("Enrollment complete. Returning to search loop.\r\n");
    }
    prev_button_state = button_state;

    uint16_t matched_page = 0;
    HAL_StatusTypeDef status = Fingerprint_CaptureAndSearch(&matched_page);

    if (status == HAL_OK)
    {
      Fingerprint_ReportMatch(matched_page);
    }
    else if (status == HAL_TIMEOUT)
    {
      Fingerprint_HandleNoMatch();
    }

    HAL_Delay(200);
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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ETH Initialization Function
  * @param None
  * @retval None
  */
static void MX_ETH_Init(void)
{

  /* USER CODE BEGIN ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

   static uint8_t MACAddr[6];

  /* USER CODE BEGIN ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x80;
  MACAddr[2] = 0xE1;
  MACAddr[3] = 0x00;
  MACAddr[4] = 0x00;
  MACAddr[5] = 0x00;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1524;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  if (HAL_ETH_Init(&heth) != HAL_OK)
  {
    Error_Handler();
  }

  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  /* USER CODE BEGIN ETH_Init 2 */

  /* USER CODE END ETH_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 57600;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PG9 PG14 */
  GPIO_InitStruct.Pin = FP_UART_RX_Pin|FP_UART_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

static void Fingerprint_Announce(const char *message)
{
  HAL_UART_Transmit(&huart6, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
}

static HAL_StatusTypeDef Fingerprint_SendCommand(uint8_t instruction,
                                                const uint8_t *payload,
                                                uint16_t payload_len,
                                                uint8_t *ack_buf,
                                                uint16_t ack_buf_len,
                                                uint16_t *out_len)
{
  uint16_t length = payload_len + 2U; /* instruction byte + checksum */
  uint16_t idx = 0;
  uint16_t checksum = 0;

  if ((payload_len + 12U) > sizeof(fp_tx_buffer))
  {
    return HAL_ERROR;
  }

  fp_tx_buffer[idx++] = (FP_START_CODE >> 8) & 0xFF;
  fp_tx_buffer[idx++] = FP_START_CODE & 0xFF;

  fp_tx_buffer[idx++] = (FP_DEFAULT_ADDRESS >> 24) & 0xFF;
  fp_tx_buffer[idx++] = (FP_DEFAULT_ADDRESS >> 16) & 0xFF;
  fp_tx_buffer[idx++] = (FP_DEFAULT_ADDRESS >> 8) & 0xFF;
  fp_tx_buffer[idx++] = FP_DEFAULT_ADDRESS & 0xFF;

  fp_tx_buffer[idx++] = FP_PACKET_COMMAND;
  fp_tx_buffer[idx++] = (length >> 8) & 0xFF;
  fp_tx_buffer[idx++] = length & 0xFF;
  fp_tx_buffer[idx++] = instruction;

  checksum = FP_PACKET_COMMAND + fp_tx_buffer[7] + fp_tx_buffer[8] + instruction;

  for (uint16_t i = 0; i < payload_len; ++i)
  {
    fp_tx_buffer[idx++] = payload[i];
    checksum += payload[i];
  }

  fp_tx_buffer[idx++] = (checksum >> 8) & 0xFF;
  fp_tx_buffer[idx++] = checksum & 0xFF;

  HAL_StatusTypeDef status = HAL_UART_Transmit(&huart6, fp_tx_buffer, idx, FP_TIMEOUT_MS);
  if (status != HAL_OK)
  {
    return (status == HAL_TIMEOUT) ? HAL_TIMEOUT : HAL_ERROR;
  }

  status = HAL_UART_Receive(&huart6, ack_buf, 9U, FP_TIMEOUT_MS);
  if (status != HAL_OK)
  {
    return (status == HAL_TIMEOUT) ? HAL_TIMEOUT : HAL_ERROR;
  }

  uint16_t ack_len = ((uint16_t)ack_buf[7] << 8) | ack_buf[8];
  if ((ack_len + 9U) > ack_buf_len)
  {
    return HAL_ERROR;
  }

  status = HAL_UART_Receive(&huart6, ack_buf + 9U, ack_len, FP_TIMEOUT_MS);
  if (status != HAL_OK)
  {
    return (status == HAL_TIMEOUT) ? HAL_TIMEOUT : HAL_ERROR;
  }

  if (out_len != NULL)
  {
    *out_len = ack_len;
  }

  return HAL_OK;
}

static HAL_StatusTypeDef Fingerprint_VerifyPassword(void)
{
  uint8_t payload[4] = {0x00, 0x00, 0x00, 0x00};
  uint16_t ack_len = 0;
  HAL_StatusTypeDef status = Fingerprint_SendCommand(0x13, payload, sizeof(payload), fp_rx_buffer, sizeof(fp_rx_buffer), &ack_len);
  if (status != HAL_OK)
  {
    return status;
  }

  uint8_t confirm_code = fp_rx_buffer[9];
  return (confirm_code == FP_OK) ? HAL_OK : HAL_ERROR;
}

static HAL_StatusTypeDef Fingerprint_GetImage(void)
{
  uint16_t ack_len = 0;
  HAL_StatusTypeDef status = Fingerprint_SendCommand(0x01, NULL, 0, fp_rx_buffer, sizeof(fp_rx_buffer), &ack_len);
  if (status != HAL_OK)
  {
    return status;
  }

  uint8_t confirm_code = fp_rx_buffer[9];
  if (confirm_code == FP_NO_FINGER)
  {
    return HAL_BUSY;
  }
  return (confirm_code == FP_OK) ? HAL_OK : HAL_ERROR;
}

static HAL_StatusTypeDef Fingerprint_Image2Tz(uint8_t buffer_id)
{
  uint16_t ack_len = 0;
  uint8_t payload[1] = {buffer_id};
  HAL_StatusTypeDef status = Fingerprint_SendCommand(0x02, payload, sizeof(payload), fp_rx_buffer, sizeof(fp_rx_buffer), &ack_len);
  if (status != HAL_OK)
  {
    return status;
  }
  return (fp_rx_buffer[9] == FP_OK) ? HAL_OK : HAL_ERROR;
}

static HAL_StatusTypeDef Fingerprint_Search(uint16_t *page_id)
{
  uint8_t payload[6] = {0x01, 0x00, 0x00, 0x00, 0x00, 0xA2};
  uint16_t ack_len = 0;
  HAL_StatusTypeDef status = Fingerprint_SendCommand(0x04, payload, sizeof(payload), fp_rx_buffer, sizeof(fp_rx_buffer), &ack_len);
  if (status != HAL_OK)
  {
    return status;
  }

  uint8_t confirm_code = fp_rx_buffer[9];
  if (confirm_code == FP_OK)
  {
    *page_id = ((uint16_t)fp_rx_buffer[10] << 8) | fp_rx_buffer[11];
    return HAL_OK;
  }

  if (confirm_code == FP_NOMATCH)
  {
    return HAL_TIMEOUT;
  }

  return HAL_ERROR;
}

static HAL_StatusTypeDef Fingerprint_CaptureAndSearch(uint16_t *page_id)
{
  HAL_StatusTypeDef status = Fingerprint_GetImage();
  if (status != HAL_OK)
  {
    return status;
  }

  if (Fingerprint_Image2Tz(0x01) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return Fingerprint_Search(page_id);
}

static void Fingerprint_EnrollDatabase(void)
{
  for (size_t i = 0; i < (sizeof(kFingerDatabase) / sizeof(kFingerDatabase[0])); ++i)
  {
    const FingerEntry_t *entry = &kFingerDatabase[i];
    char msg[96];

    snprintf(msg, sizeof(msg), "Starting enrollment for %s (ID %u).\r\n", entry->label, entry->page_id);
    Fingerprint_Announce(msg);

    if (Fingerprint_Enroll(entry->page_id) == HAL_OK)
    {
      snprintf(msg, sizeof(msg), "Enrollment successful for %s at page %u.\r\n", entry->label, entry->page_id);
      Fingerprint_Announce(msg);
    }
    else
    {
      snprintf(msg, sizeof(msg), "Enrollment FAILED for %s (page %u). Retrying later may be required.\r\n", entry->label, entry->page_id);
      Fingerprint_Announce(msg);
    }

    HAL_Delay(500);
  }
}

static void Fingerprint_PromptStartupEnrollment(void)
{
  Fingerprint_Announce("Hold the USER button to enroll all configured fingerprints. Release to skip.\r\n");

  /* Small delay to allow the button to be pressed after reset */
  HAL_Delay(500);

  if (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET)
  {
    Fingerprint_Announce("User button detected at startup. Beginning enrollment sequence...\r\n");
    Fingerprint_EnrollDatabase();
    Fingerprint_Announce("Startup enrollment complete.\r\n");
  }
  else
  {
    Fingerprint_Announce("Skipping startup enrollment. You can trigger it later with the USER button.\r\n");
  }
}

static HAL_StatusTypeDef Fingerprint_Enroll(uint16_t page_id)
{
  char msg[80];
  uint8_t payload[4];

  snprintf(msg, sizeof(msg), "Place finger for enrollment ID %u...\r\n", page_id);
  Fingerprint_Announce(msg);
  while (Fingerprint_GetImage() == HAL_BUSY)
  {
    HAL_Delay(100);
  }

  if (Fingerprint_Image2Tz(0x01) != HAL_OK)
  {
    return HAL_ERROR;
  }

  Fingerprint_Announce("Remove finger...\r\n");
  HAL_Delay(1500);
  Fingerprint_Announce("Place the same finger again...\r\n");
  while (Fingerprint_GetImage() == HAL_BUSY)
  {
    HAL_Delay(100);
  }

  if (Fingerprint_Image2Tz(0x02) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (Fingerprint_SendCommand(0x05, NULL, 0, fp_rx_buffer, sizeof(fp_rx_buffer), NULL) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (fp_rx_buffer[9] != FP_OK)
  {
    return HAL_ERROR;
  }

  payload[0] = 0x01;
  payload[1] = (page_id >> 8) & 0xFF;
  payload[2] = page_id & 0xFF;
  payload[3] = 0x00; /* default permission */

  if (Fingerprint_SendCommand(0x06, payload, 4U, fp_rx_buffer, sizeof(fp_rx_buffer), NULL) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return (fp_rx_buffer[9] == FP_OK) ? HAL_OK : HAL_ERROR;
}

static void Fingerprint_ReportMatch(uint16_t page_id)
{
  const char *label = "Unknown user";
  for (size_t i = 0; i < (sizeof(kFingerDatabase) / sizeof(kFingerDatabase[0])); ++i)
  {
    if (kFingerDatabase[i].page_id == page_id)
    {
      label = kFingerDatabase[i].label;
      break;
    }
  }

  char msg[96];
  snprintf(msg, sizeof(msg), "Fingerprint match: %s (ID %u)\r\n", label, page_id);
  Fingerprint_Announce(msg);

  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
}

static void Fingerprint_HandleNoMatch(void)
{
  Fingerprint_Announce("Fingerprint not recognized.\r\n");
  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
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
