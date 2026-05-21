 #ifndef HT1602_H                          
 #define HT1602_H
    
#include "main.h"

#endif
#define Ht1621_CS_0        (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET))
#define Ht1621_CS_1        (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET))

#define Ht1621_RD_0        (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET))
#define Ht1621_RD_1        (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET))

#define Ht1621_WR_0        (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET))
#define Ht1621_WR_1        (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET))

#define Ht1621_Data_0      (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET))
#define Ht1621_Data_1      (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET))

#define TOP_Light_ON()        (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET))
#define TOP_Light_OFF()       (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET))

#define Light_ON()        (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET))
#define Light_OFF()       (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET))

#define Cmd8      0x80
#define Cmd9      9
#define BIAS      0X52  //1/3ƫѹ��4����
#define RC256     0X28  //ϵͳʱ��ѡ��ΪƬ��RC����
#define WDTDIS1   0X0A  //��ֹWDT�����־���
#define TIMERDIS  0X08  //ʱ���������
#define SYSEN     0X02  //��ϵͳʱ��
#define LCDON     0X06  //��ƫѹ������
#define LCDOFF    0X04  //��ƫѹ������
#define SYSDIS    0X00  //�ر�ϵͳʱ�Ӻ�ƫѹ������

void Delay(unsigned int i);
void SendBit(unsigned char dat,unsigned char bitcnt);
void HT1621_init(void);
void HT1621_WriteCmd(unsigned char Cmd,unsigned char Cmd_Data);
void HT1621_WriteData(unsigned char Waddr,unsigned char *Wdata,unsigned char Wnum);
void HT1621_Cmd_init(void);
void Signle_Dis(unsigned char x);
void Point_Dis(unsigned char x);
void TT_Dis(unsigned char x);
void SEG1_Dis(unsigned char x);
void SEG2_Dis(unsigned char x);
void SEG3_Dis(unsigned char x);
void DisplayFloat(float value);
void Dis_Clr(void);


