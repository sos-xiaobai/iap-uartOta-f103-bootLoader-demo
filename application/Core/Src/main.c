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
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bootloader.h"
#include "bootloader_uart.h"
#include "wifi.h"
#include "HT1602.H"
#include "TouchIN.h"
#include "as5600.h"
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
uint8_t temp_wifi_uart_rx_buf[256];  /* 串口接收缓冲�???????? */
unsigned char dismem[16]={0X0,0X0,0X0,0X0,0X0,0X0,0X0,0X0,0X0,0X0,0X0,0X0,0X0,0X0,0X00,0X00};
float now_angle;
float last_angle;
unsigned long target_angle;
uint8_t dis_angle;
uint8_t alive;
uint32_t dis_direction = 0;  //ui显示的方向 0/1 持续化存储在flash
AS5600_TypeDef as5600;
uint8_t control_origin = 0; //0:控制来自按键 1:控制来自app 默认为0
uint8_t sleep_flag = 0; //睡眠状态标志，0:正常状态 1:睡眠状态
uint8_t my_reset_wifi_flag = 0; //wifi重置标志，0:正常状态 1:正在重置wifi，此时不处理按键逻辑和显示内容，等待wifi重置完成后重启设备
uint16_t left_right_count = 0; //左右按键同时按下计数
uint16_t middle_count = 0;  //中间按键按下计数
uint16_t left_right_middle_count = 0; //三个按键全部按下计数
uint16_t key_no_touch_count = 0; //没有按键按下计数
uint8_t dp_angle_update_flag = 0; //dp上报角度标志，0:不需要上报 1:需要上报
uint8_t dp_wifi_update_flag = 0; //dp上报wifi信号强
uint16_t dp_wifi_update_count = 0; //dp上报wifi信号强计数，达到一定次数后触发上报一次
//realtime open value(只上报)
//备注:
#define DPID_ANGLEDIS 102
//wifi信号强度(只上报)
//备注:
#define DPID_WIFI_VALUE 103
uint8_t test_direc = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t AngleToDis(float angle);
uint8_t test = 0;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
 {

  /* USER CODE BEGIN 1 */
	
	/* 设置中断向量表偏- 必须在最前面 ota�??要解注释，调试可以保�?? */
  SCB->VTOR = APP_START_ADDR;
	/* 编译成bootloader�??支持的bin，需要修改ld中的以下参数*/
	// flash 0x08004000  0xBC00
  // RAM   0x20001000  0x4000
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
  MX_TIM4_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  /* 初始化Bootloader */
  Bootloader_Init();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET); 
  /* 清空UART缓冲区，避免之前的数据干扰 */
  __HAL_UART_FLUSH_DRREGISTER(&huart1);
  __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);
  __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_TC);
  HAL_UART_Receive_IT(&huart1, temp_wifi_uart_rx_buf, 1);  // 启动UART接收中断，数据由MCU_SDK处理
  /* 初始化串口IAP协议 */
  Bootloader_UART_Init(&huart1);
	
	//Bootloader_UART_SendString("  STM32 Application enter successfull!\r\n");

  /* 初始化wifi协议,必须在MCU初始化代码中调用该函�???????? */
  wifi_protocol_init();  
  // 产品信息 确保在上�????????次ota结束重启后，bootloader能回复模块当前的产品信息，防止误判为ota超时
  Bootloader_UpdateAppVersion();
  extern void product_info_update();
  product_info_update();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	as5600.i2cHandle = &hi2c2;
	as5600.i2cAddr = AS5600_SLAVE_ADDRESS<<1; // STM32 HAL库使用左移后的地�???
  AS5600_Init(&as5600);
  HT1621_Cmd_init();//液晶初始化sun
  Dis_Clr();        //清屏sun
  TT_Dis(1);
  Signle_Dis(1);
  HT1621_WriteData(0x00,dismem,16);
  TOP_Light_ON();
  Light_OFF();
  extern unsigned char TouchIN;  
  //读取显示方向状态，确保在每次上电，app能正确读取显示方向状态
  Bootloader_ReadAppStatus(&dis_direction); 	
  HAL_TIM_Base_Start_IT(&htim4); 
  HAL_Delay(100); // 等待系统稳定 
  while (1)
  {
    // WBR模块通信
    wifi_uart_service();
    // 磁编读取角度
    AS5600_Get_True_Angle(&as5600, &now_angle);    

    if(sleep_flag){
      Dis_Clr();        //清屏sun
      HT1621_WriteData(0x00,dismem,16);
      Light_OFF();
      continue;    // 如果睡眠状态,不处理按键逻辑和显示内容
    }
		
    TT_Dis(1);
    HT1621_WriteData(0x00,dismem,16);

    // 不是只在睡眠状态检查和更新UI显示方向标志-->flash持久化存储 防止写flash导致中断失败
    uint32_t flash_dis_direction  = 2;
    if(Bootloader_ReadAppStatus(&flash_dis_direction) == IAP_SUCCESS)
    {
        if(flash_dis_direction != dis_direction){
            Bootloader_SaveAppStatus(dis_direction);
        }
    }

    // 根据control_origin判断是手机角度or按键控制
    if(control_origin == 1){ // 如果控制来自app，则根据目标角度和当前角度的差值来控制转动，避免频繁切换转动状态
      if(target_angle >= 0 && target_angle <= 180){
        if(target_angle < now_angle - 0.5){ // 角度差大于0.5度才转动
          Turn_left();
          test_direc = 1;
        }else if(target_angle > now_angle + 0.5){
          Turn_right();
          test_direc = 2;
        }else{
          Turn_stop();
          test_direc = 0;
        }
      }
    }else{
      // 根据按键状和角度信息更新显示内容
      // 只按住左键or右键
      if(TouchIN == 1 || TouchIN == 2){
        if(TouchIN == 1){
          if(now_angle>=0){
            Turn_left(); //左转
          }else{
            Turn_stop();// 不按住左or右键位,关闭转动
          }
        }else if(TouchIN == 2){
          if(now_angle<=180){
            Turn_right(); //右转
          }else{
            Turn_stop();
          }
        }
      }else{
        Turn_stop();// 不按住左or右键位,关闭转动
      }
    }

    // 判断显示方向
    if(dis_direction){    
        //TT_Dis(dis_direction); 
        dis_angle = AngleToDis(now_angle);
    }else{
      //TT_Dis(dis_direction);
      dis_angle = 51 - AngleToDis(now_angle);
    }    
    //HT1621_WriteData(0x00,dismem,16);

    // wifi重置
    if(my_reset_wifi_flag){

        if(reset_wifi_flag)
        {
            my_reset_wifi_flag = 0;
        }else{
					// 重置wifi
					mcu_reset_wifi();
				}
    }

    // 更新wifi状态 只要不是低功耗模式就点亮wifi
    if(wifi_work_state == 4){
      Signle_Dis(1);
    }else{
      Signle_Dis(0);
    }
    HT1621_WriteData(0x00,dismem,16);

    // 只要联网就上报角度数据
    if(wifi_work_state == 4 || wifi_work_state == 3){
      if(dp_angle_update_flag){
        mcu_dp_value_update(DPID_ANGLE,(unsigned long)(now_angle)); //VALUE型数据上报 数值范围: 0-180, 间距: 1, 倍数: 0, 单位: °;
        mcu_dp_value_update(DPID_ANGLEDIS,(unsigned long)(now_angle)); //VALUE型数据上报数值范围: 0-180, 间距: 1, 倍数: 0, 单位:°;
        dp_angle_update_flag = 0;
      }
      if(dp_wifi_update_flag){
        dp_wifi_update_flag = 0;
        mcu_dp_value_update(DPID_WIFI_VALUE,(unsigned long)(wifi_rssi)); //VALUE型数据上报 数值范围:-500-500, 间距: 1, 倍数: 0, 单位: dBm;
      }
    }

    // 更新显示
    Point_Dis(dis_angle);
    HT1621_WriteData(0x00,dismem,16);
    DisplayFloat(now_angle);
    HT1621_WriteData(0x00,dismem,16);
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

// 200hz
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == (&htim4))
    {
      TouchIN_Dect(); // 触摸按键�?�?

      // ************判断盖子开合 启动休眠  待测试************//
      // if(GetCoverStatus()){
      //   sleep_flag = 1;
      // }else{
      //   sleep_flag = 0;
      // }


      // ************判断左右长按************//
      if(TouchIN == 3){
        left_right_count++;
      }else{
        left_right_count = 0;
      }
      // 长按3s 切换方向
      if(left_right_count == 600){
        left_right_count = 0;
        dis_direction = dis_direction==1?0:1;
      }

      // ************判断中间长按************//
      if(TouchIN == 4){
        middle_count++;
      }else{
        middle_count = 0;
      }
      // 长按5s 重置wifi
      if(middle_count == 1000){
        middle_count = 0;
        my_reset_wifi_flag = 1;
      }

      // ************判断三键长按************//
      if(TouchIN == 7){
        left_right_middle_count++;
      }else{
        left_right_middle_count = 0;
      }
      // 长按3s 休眠
      if(left_right_middle_count == 600){
        left_right_middle_count = 0;
        sleep_flag = sleep_flag==1?0:1;
      }
      // ************检测按键 控制key的灯光************//
      if(TouchIN == 0){
        key_no_touch_count++;
      }else{
        control_origin = 0; // 只要有按键操作就认为控制来自按键
        Light_ON();
        key_no_touch_count = 0;
      }
      if(key_no_touch_count >= 7000){
        Light_OFF();
      }
     // ************wifi 强度/阀门角度 检测更新************//
      dp_wifi_update_count++;
      if(dp_wifi_update_count >= 15){ // 每1s更新一次wifi信号强度和角度数据，避免频繁上报
        dp_wifi_update_count = 0;
          mcu_get_wifi_rssi(); // 获取wifi信号强度，结果会通过uart接收中断回调函数返回，处理完后会置位dp_wifi_update_flag标志位
        //mcu_start_wifitest();
        dp_angle_update_flag = 1;
      }
      if(dp_angle_update_flag == 0){ // 角度变化时也更新角度，避免频繁上报
        if(now_angle != last_angle){ 
          dp_angle_update_flag = 1;
        }
      }
      last_angle = now_angle;
    }
}


uint8_t AngleToDis(float angle) {
    // 角度范围 0~180，输出范围 0~51（共 52 个等级）
    // 映射公式：output = floor(angle * 52 / 180)，边界处理 angle=180 时输出 51
    uint16_t tmp = angle * 52u;
    uint8_t result = tmp / 180u;
    if (result == 52u) {   // 仅当 angle=180 时 result 为 52
        result = 51u;
    }
    return result;
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
