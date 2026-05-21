/*=======================TouchIn.c========================
 * （1）将TouchIN_Dect()函数放置于定时中断中，就可以将触摸按键变成普通IO按键使用。
 * （2）全局变量TouchIN的作用相当于PxIN寄存器。
 * （3）此文件已移植为基于 STM32 HAL 的 GPIO 读取实现。
 *    按键映射为 PB15、PB14、PB13，按下为低电平（active low）。
 *  Created on: 2013-2-20 (modified)
 *      Author: Administrator
 *======================================================*/
#include "main.h"
#include "stm32f1xx_hal.h"
#define KEY_NUM            3            /*触摸按键数目，根据需要修改*/
// 按键映射：PB15, PB14, PB13
GPIO_TypeDef* KEY_GPIO[KEY_NUM] = {GPIOB, GPIOB, GPIOB};
const uint16_t KEY_PIN[KEY_NUM] = {GPIO_PIN_13, GPIO_PIN_15, GPIO_PIN_14};
//==================静态全局变量============================
unsigned char Key_Buff[KEY_NUM][4] = {{0}};  // 软件FIFO
uint8_t Status[3] = {0}; // 用于记录按键状态，0表示未按下，1表示按下
//===============全局变量，如有需要可移植到Global.h统一管理================
unsigned char TouchIN;                                            // 相当于PxIN寄存器作用，支持8个触摸按键

/******************************************************************************************************
* 名       称：Key_FIFO_Update()
* 功       能：读取指定按键电平并更新4字节软件FIFO（按下为低电平）
* 入口参数：idx - 按键索引
* 出口参数：无
******************************************************************************************************/
void Key_FIFO_Update(int idx)
{
    // Key_Buff[idx][0] = Key_Buff[idx][1];
    // Key_Buff[idx][1] = Key_Buff[idx][2];
    // Key_Buff[idx][2] = Key_Buff[idx][3];
    // GPIO_PinState state = HAL_GPIO_ReadPin(KEY_GPIO[idx], KEY_PIN[idx]);
    // // 按下为高电平 -> 记为1
    // if (state == GPIO_PIN_SET)
    //     Key_Buff[idx][3] = 1;
    // else
    //     Key_Buff[idx][3] = 0;
    GPIO_PinState state = HAL_GPIO_ReadPin(KEY_GPIO[idx], KEY_PIN[idx]);
	Status[idx] = (state == GPIO_PIN_SET) ? 1 : 0; // 更新按键状态，按下为1，松开为0
    // 按下为高电平 -> 记为1
    if (state == GPIO_PIN_SET)
        Key_Buff[idx][0] = 1;
    else
        Key_Buff[idx][0] = 0;
}

/******************************************************************************************************
* 名       称：Key_FIFO()
* 功       能：兼容旧接口，实际逻辑由 Key_FIFO_Update 完成
******************************************************************************************************/
void Key_FIFO()
{
    // 保留空实现以兼容调用
}

/******************************************************************************************************
* 名       称：Key_Judge_Index()
* 功       能：按键抗干扰仲裁。只有连续4次都识别到按键才算按下，
*           只有连续4次都识别不到按键才算松开。
* 入口参数：idx - 按键索引
******************************************************************************************************/
void Key_Judge_Index(int idx)
{
    // if ((Key_Buff[idx][0] == 0) && (Key_Buff[idx][1] == 0)
    //     && (Key_Buff[idx][2] == 0) && (Key_Buff[idx][3] == 0))
    //     TouchIN &= ~(1 << idx); // 按键松开
    // if ((Key_Buff[idx][0] == 1) && (Key_Buff[idx][1] == 1)
    //     && (Key_Buff[idx][2] == 1) && (Key_Buff[idx][3] == 1))
    //     TouchIN |= (1 << idx); // 按键按下
	
	if(Key_Buff[idx][0] == 1) {
		TouchIN |= (1 << idx); // 按键按下
	} else {
		TouchIN &= ~(1 << idx); // 按键松开
	}
}

/******************************************************************************************************
* 名       称：TouchIN_Dect()
* 功       能：触摸按键检测。
* 入口参数：无
* 出口参数：无
* 说     明 ： 在定时中断内调用该函数。调用后，全局变量 TouchIN 就相当于 PxIN。
******************************************************************************************************/
void TouchIN_Dect()                                            // 触摸输入检测（改为 GPIO 读取）
{
    for (int i = 0; i < KEY_NUM; i++) {
        Key_FIFO_Update(i);
        Key_Judge_Index(i);
    }
}

// 左转
void Turn_left(){
    HAL_GPIO_WritePin(INA_GPIO_Port,INA_Pin,GPIO_PIN_SET);
    HAL_GPIO_WritePin(INB_GPIO_Port,INB_Pin,GPIO_PIN_RESET);
}

// 右转
void Turn_right(){
    HAL_GPIO_WritePin(INA_GPIO_Port,INA_Pin,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(INB_GPIO_Port,INB_Pin,GPIO_PIN_SET);
}

// 关闭
void Turn_stop(){
    HAL_GPIO_WritePin(INA_GPIO_Port,INA_Pin,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(INB_GPIO_Port,INB_Pin,GPIO_PIN_RESET);    
}

// 获取盖子开合状态
GPIO_PinState GetCoverStatus(){
    return (HAL_GPIO_ReadPin(COVER_IO_GPIO_Port,COVER_IO_Pin));
}