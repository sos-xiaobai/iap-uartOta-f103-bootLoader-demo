/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bootloader.h"
#include "bootloader_uart.h"
#include "wifi.h"
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

/* USER CODE BEGIN PV */
uint8_t temp_wifi_uart_rx_buf[256];  /* 串口接收缓冲区 */
uint8_t enter_iap = 0;  /* 进入IAP模式标志 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  /* 初始化Bootloader */
  Bootloader_Init();
  
  /* 清空UART缓冲区，避免之前的数据干�? */
  __HAL_UART_FLUSH_DRREGISTER(&huart1);
  __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);
  __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_TC);
  HAL_UART_Receive_IT(&huart1, temp_wifi_uart_rx_buf, 1);  // 启动UART接收中断，数据由MCU_SDK处理
  /* 初始化串口IAP协议 */
  Bootloader_UART_Init(&huart1);
  
  /* 初始化wifi协议,必须在MCU初始化代码中调用该函数 */
  wifi_protocol_init(); 

  /* 发�?�启动信�? */
  Bootloader_UART_SendString("\r\n");
  Bootloader_UART_SendString("====================================\r\n");
  Bootloader_UART_SendString("  STM32 IAP Bootloader v1.0.0\r\n");
  Bootloader_UART_SendString("  MCU: STM32F103C8\r\n");
  Bootloader_UART_SendString("====================================\r\n");
  
  /* ========== 判断是否进入IAP升级模式 ========== */
  //uint8_t enter_iap = 0;  /* 进入IAP模式标志 */
  /* 方式1: �?查是否有未完成的升级标志 */
  if (Bootloader_CheckUpdateFlag())
  {
      Bootloader_UART_SendString("WARNING: Last update incomplete!\r\n");
      Bootloader_ClearUpdateFlag();
      enter_iap = 1;
  }
  
  /* 方式2: �?查APP是否有效 */
  if (!enter_iap && !Bootloader_CheckAppValid())
  {
      Bootloader_UART_SendString("Application invalid!\r\n");
      enter_iap = 1;
  }
  
  /* 方式3: GPIO按键触发（可选，�?要配置对应的GPIO�?*/
  /* 取消下面的注释来启用按键触发功能�?
   * 1. 在CubeMX中配置一个GPIO输入引脚（如PA0），设置为上拉输�?
   * 2. 修改下面的GPIO_Port和GPIO_Pin为实际配置的引脚
   * 3. 按住按键上电即可进入IAP模式
   */
  /*
  if (!enter_iap && HAL_GPIO_ReadPin(BOOT_KEY_GPIO_Port, BOOT_KEY_Pin) == GPIO_PIN_RESET)
  {
      Bootloader_UART_SendString("Button pressed, enter IAP mode...\r\n");
      enter_iap = 1;
  }
  */
  
  /* 方式4: 超时等待 - 等待firmware命令 */
  if (!enter_iap)
  {
      Bootloader_UART_SendString("\r\n");
      Bootloader_UART_SendString("Wait Firmware, or wait to run APP...\r\n");
      Bootloader_UART_SendString("Waiting ");
      
      /* 等待10秒，等待是否收到升级命令 */
      uint32_t wait_time = 10000;  /* 等待时间（毫秒）*/
      uint32_t start_tick = HAL_GetTick();
      
      while ((HAL_GetTick() - start_tick) < wait_time)
      {
          /*阻塞等待开启固件升级命令*/
          wifi_uart_service();
          if(enter_iap) break;
          ///HAL_Delay(10);
      }
  }
  
  /* ========== 根据标志决定是进入IAP还是跳转APP ========== */
  if (enter_iap)
  {
      /* 进入IAP升级模式 */
      Bootloader_UART_SendString("\r\n");
      Bootloader_UART_SendString("==================================================\r\n");
      Bootloader_UART_SendString("  Entering IAP Mode\r\n");
      Bootloader_UART_SendString("==================================================\r\n");
      Bootloader_UART_SendString("Waiting for firmware...\r\n");
      Bootloader_UART_SendString("\r\n");
  }
  else
  {
      /* 跳转到应用程�? */
      if (Bootloader_CheckAppValid())
      {
          /* 显示版本信息（如果有�?*/
          if (Bootloader_IsAppVersionValid())
          {
              Firmware_VersionTypeDef app_version;
              if (Bootloader_ReadAppVersion(&app_version) == IAP_SUCCESS)
              {
                  char version_str[64];
                  snprintf(version_str, sizeof(version_str), 
                          "APP Version: %d.%d.%d\r\n", 
                          app_version.major, app_version.minor, app_version.patch);
                  Bootloader_UART_SendString(version_str);
                  
                  if (app_version.description[0] != '\0')
                  {
                      Bootloader_UART_SendString("Description: ");
                      Bootloader_UART_SendString(app_version.description);
                      Bootloader_UART_SendString("\r\n");
                  }
              }
          }
          
          Bootloader_UART_SendString("Jumping to application...\r\n");
          HAL_Delay(100);
          Bootloader_JumpToApp();
          
          /* 如果跳转失败，会继续执行到这�? */
          Bootloader_UART_SendString("ERROR: Jump to application failed!\r\n");
      }
      else
      {
          /* 理论上不会到这里，因为前面已经检查过�? */
          Bootloader_UART_SendString("ERROR: Application invalid!\r\n");
      }
      
      /* 跳转失败，进入IAP模式 */
      Bootloader_UART_SendString("Enter IAP mode...\r\n");
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 处理串口IAP协议 */
    /* 该函数会阻塞等待串口数据，超时后返回 */
    /* 支持的命令：
     * - IAP_CMD_GET_INFO      (0xA4): 获取Bootloader信息
     * - IAP_CMD_START_UPDATE  (0xA5): �?始升�?
     * - IAP_CMD_WRITE         (0xA1): 写入固件数据
     * - IAP_CMD_END_UPDATE    (0xA6): 结束升级
     * - IAP_CMD_VERIFY        (0xA7): 校验固件
     * - IAP_CMD_JUMP          (0xA3): 跳转到应用程�?
     */

    wifi_uart_service();
    //Bootloader_UART_Process();
    
    /* LED闪烁指示Bootloader运行状�?�（可�?�） */
    /* 取消注释下面的代码来启用LED指示�?
    static uint32_t led_tick = 0;
    if (HAL_GetTick() - led_tick > 500)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        led_tick = HAL_GetTick();
    }
    */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1) 
  {
	  HAL_UART_IRQHandler(&huart1);	  
    uart_receive_input(temp_wifi_uart_rx_buf[0]);  // 将接收到的数据传递给MCU_SDK处理
    HAL_UART_Receive_IT(&huart1,temp_wifi_uart_rx_buf,1);
  }
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
