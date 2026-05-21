/*
 * TouchIN.h
 *
 *  Created on: 2013-2-20
 *      Author: Administrator
 */

#ifndef TOUCHIN_H_
#define TOUCHIN_H_

extern void TouchIN_Dect()	;												
extern unsigned char TouchIN;	

// 左转
void Turn_left();
// 右转
void Turn_right();
// 关闭
void Turn_stop();
// 获取盖子开合状态
GPIO_PinState GetCoverStatus();
//extern unsigned int TouchIN;											


#endif /* TOUCHIN_H_ */
