#include "HT1602.H" 
//#include <msp430.h>

const unsigned char Tablep[11]={0xf5,0x05,0xd3,0x97,0x27,0xb6,0xf6,0x15,0xf7,0xb7,0x00 } ;
extern unsigned char dismem[16];
extern unsigned char water_temp_now;
extern unsigned char water_temp_dis;
extern unsigned char TPM2_S;
extern unsigned int Beep_Time;
extern unsigned char Beep_Ok;
extern unsigned char time_set[2];
extern unsigned char DI2_QS;

void Delay(unsigned int i)
{
    //HAL_Delay(i);
    uint16_t count = i*10;
    while(count--);
} 

void SendBit(unsigned char dat,unsigned char bitcnt)
{
 unsigned char i;
 for(i=0;i<bitcnt;i++)
 {
     Ht1621_WR_0; /*��ʱ����Ϊ��֪ͨ��������ʼ������λ*/

  if(( dat & 0x80 ) == 0)
  Ht1621_Data_0;
  else
  Ht1621_Data_1; /*���������ɸ�λ����λ����*/

  Delay(1);
  Ht1621_WR_1; /*ǯס����׼����һ������λ*/
  Delay(1);
  dat=dat<<1;   /*������������һλ*/
 }
}
void HT1621_init(void)
{

 Ht1621_CS_1;   /*CS �˿�Ϊ1*/
 Ht1621_WR_1;   /*WR �˿�Ϊ1*/
 Ht1621_RD_1;   /*WR �˿�Ϊ1*/
 Ht1621_Data_1;   /*DATA�˿�Ϊ1*/

}


void HT1621_WriteCmd(unsigned char Cmd,unsigned char Cmd_Data)
{
 Ht1621_CS_0;
 Delay(1);
 SendBit(Cmd,4);
 SendBit(Cmd_Data,8);
 Ht1621_CS_1;
 Delay(1);
}

 

void HT1621_WriteData(unsigned char Waddr,unsigned char *Wdata,unsigned char Wnum)
{
 

 unsigned char i=0;
  //DisableInterrupts;
 Ht1621_CS_0;
 Delay(1);
 SendBit(0xA0,3);
 Waddr = Waddr << 2;
 SendBit(Waddr,6);
 //Wdata = Wdata << (8-Wnum);
 for(i=0;i<Wnum;i++){
  
     SendBit(Wdata[i],8);
     Delay(1);  
 }
 Ht1621_CS_1;
 Delay(1);
 //EnableInterrupts;
}

void HT1621_Cmd_init(void)
{
  HT1621_WriteCmd(Cmd8,SYSEN); //��ϵͳ��������
  HT1621_WriteCmd(Cmd8,LCDON); //��LCDƫѹ����������
 // HT1621_WriteCmd(Cmd8,WDTDIS); //WDT �����־���ʧЧ����
  HT1621_WriteCmd(Cmd8,TIMERDIS ); //ʱ�����ʹ������
 // HT1621_WriteCmd(Cmd8,RC256 ); ///ϵͳʱ��ԴƬ��RC��������
  HT1621_WriteCmd(Cmd8, BIAS ); //LCD1/3ƫѹѡ��4������������
  //HT1621_WriteCmd(Cmd9, F1 ); /*ʱ��/WDT ʱ�����1Hz WDT����Ϊ4 �����*/
  //HT1621_WriteCmd(Cmd9, IRQEN ); /*ʱ��/WDT ʱ�����1Hz WDT����Ϊ4 �����*/
}
void Signle_Dis(unsigned char x){

    if(x==0){
        dismem[0] = dismem[0]&0xfe;
    }else{
        dismem[0] = dismem[0]|0x01;
    }
}
void TT_Dis(unsigned char x){
    if(x==0){
        dismem[0] = dismem[0]&0xef;
    }else{
        dismem[0] = dismem[0]|0x10;
    }
}

/**
 * @brief  显示浮点数（0.0 ~ 180.0）到三位数码管
 * @param  value: 待显示的浮点数
 */
void DisplayFloat(float value)
{
    uint8_t num;

   /* 四舍五入并限幅到 0~180 */
   if (value < 0.0f) {
       value = 0.0f;
   } else if (value > 180.0f) {
       value = 180.0f;
   }
    num = (uint8_t)(value + 0.5f);

    /* 提取各位数字 */
    uint8_t bai = num / 100;          // 百位
    uint8_t shi = (num % 100) / 10;   // 十位
    uint8_t ge  = num % 10;           // 个位

    /* 高位不显示：只有数值 ≥100 才调百位接口 */
    if (num >= 100) {
        SEG3_Dis(bai);   /* 百位 */
    }else{
        SEG3_Dis(3); /* 3代表不显示 */
    }
    /* 数值 ≥10 才调十位接口 */
    if (num >= 10) {
        SEG2_Dis(shi);   /* 十位 */
    }else{
        SEG2_Dis(10); /* 10代表不显示 */
    }
		
    /* 个位总是显示（包括 0） */
    SEG1_Dis(ge);       /* 个位 */
}

void SEG1_Dis(unsigned char x){

    dismem[8] &= ~0x0e;
    dismem[9] &= ~0xfe;
    switch (x) {
    case 0:
        dismem[8] = dismem[8]|0x0a;
        dismem[9] = dismem[9]|0xf0;
    break;
    case 1:
        dismem[8] = dismem[8]|0x00;
        dismem[9] = dismem[9]|0x60;
    break;
    case 2:
        dismem[8] = dismem[8]|0x0c;
        dismem[9] = dismem[9]|0xb0;
    break;
    case 3:
        dismem[8] = dismem[8]|0x04;
        dismem[9] = dismem[9]|0xf0;
    break;
    case 4:
        dismem[8] = dismem[8]|0x06;
        dismem[9] = dismem[9]|0x60;
    break;
    case 5:
        dismem[8] = dismem[8]|0x06;
        dismem[9] = dismem[9]|0xd0;
    break;
    case 6:
        dismem[8] = dismem[8]|0x0e;
        dismem[9] = dismem[9]|0xd0;
    break;
    case 7:
        dismem[8] = dismem[8]|0x00;
        dismem[9] = dismem[9]|0x70;
    break;
    case 8:
        dismem[8] = dismem[8]|0x0e;
        dismem[9] = dismem[9]|0xf0;
    break;
    case 9:
        dismem[8] = dismem[8]|0x06;
        dismem[9] = dismem[9]|0xf0;
    break;
    case 10:
        dismem[8] = dismem[8]|0x00;
        dismem[9] = dismem[9]|0x00;
    break;
    default:
    }
}
void SEG2_Dis(unsigned char x){

    dismem[7] &= ~0x0e;
    dismem[8] &= ~0xf0;
    switch (x) {
    case 0:
        dismem[7] = dismem[7]|0x0a;
        dismem[8] = dismem[8]|0xf0;
    break;
    case 1:
        dismem[7] = dismem[7]|0x00;
        dismem[8] = dismem[8]|0x60;
    break;
    case 2:
        dismem[7] = dismem[7]|0x0c;
        dismem[8] = dismem[8]|0xb0;
    break;
    case 3:
        dismem[7] = dismem[7]|0x04;
        dismem[8] = dismem[8]|0xf0;
    break;
    case 4:
        dismem[7] = dismem[7]|0x06;
        dismem[8] = dismem[8]|0x60;
    break;
    case 5:
        dismem[7] = dismem[7]|0x06;
        dismem[8] = dismem[8]|0xd0;
    break;
    case 6:
        dismem[7] = dismem[7]|0x0e;
        dismem[8] = dismem[8]|0xd0;
    break;
    case 7:
        dismem[7] = dismem[7]|0x00;
        dismem[8] = dismem[8]|0x70;
    break;
    case 8:
        dismem[7] = dismem[7]|0x0e;
        dismem[8] = dismem[8]|0xf0;
    break;
    case 9:
        dismem[7] = dismem[7]|0x06;
        dismem[8] = dismem[8]|0xf0;
    break;
    case 10:
        dismem[7] = dismem[7]|0x00;
        dismem[8] = dismem[8]|0x00;
    break;
    default:
    }
}
void SEG3_Dis(unsigned char x){
       dismem[6] &= ~0x0e;
       dismem[7] &= ~0xf0;
       switch (x) {
       case 0:
           dismem[6] = dismem[6]|0x00;
           dismem[7] = dismem[7]|0x00;
       break;
       case 1:
           dismem[6] = dismem[6]|0x0a;
           dismem[7] = dismem[7]|0x00;
       break;
       default:
       }
}
void Point_Dis(unsigned char x){

    dismem[0] &= ~0x2e;//51
    dismem[1] =0;
    dismem[2] =0;
    dismem[3] =0;
    dismem[4] =0;
    dismem[5] =0;
    dismem[6] &= ~0xf1;
    dismem[7] &= ~0x01;
    dismem[8] &= ~0x01;
   switch (x) {
   case 1:
       dismem[8] = dismem[8]|0x01;
   break;
   case 2:
       dismem[7] = dismem[7]|0x01;
   break;
   case 3:
       dismem[6] = dismem[6]|0x01;
   break;
   case 4:
       dismem[6] = dismem[6]|0x10;
   break;
   case 5:
       dismem[6] = dismem[6]|0x20;
   break;
   case 6:
       dismem[6] = dismem[6]|0x40;
   break;
   case 7:
       dismem[6] = dismem[6]|0x80;
   break;
   case 8:
       dismem[5] = dismem[5]|0x08;
   break;
   case 9:
       dismem[5] = dismem[5]|0x04;
   break;
   case 10:
       dismem[5] = dismem[5]|0x02;
   break;
   case 11:
       dismem[5] = dismem[5]|0x01;
   break;
   case 12:
       dismem[5] = dismem[5]|0x10;
   break;
   case 13:
       dismem[5] = dismem[5]|0x20;
   break;
   case 14:
       dismem[5] = dismem[5]|0x40;
   break;
   case 15:
       dismem[5] = dismem[5]|0x80;
   break;
   case 16:
       dismem[4] = dismem[4]|0x08;
   break;
   case 17:
       dismem[4] = dismem[4]|0x04;
   break;
   case 18:
       dismem[4] = dismem[4]|0x02;
   break;
   case 19:
       dismem[4] = dismem[4]|0x01;
   break;
   case 20:
       dismem[4] = dismem[4]|0x10;
   break;
   case 21:
       dismem[4] = dismem[4]|0x20;
   break;
   case 22:
       dismem[4] = dismem[4]|0x40;
   break;
   case 23:
       dismem[4] = dismem[4]|0x80;
   break;
   case 24:
       dismem[3] = dismem[3]|0x08;
   break;
   case 25:
       dismem[3] = dismem[3]|0x04;
   break;
   case 26:
       dismem[3] = dismem[3]|0x02;
   break;
   case 27:
       dismem[3] = dismem[3]|0x01;
   break;
   case 28:
       dismem[3] = dismem[3]|0x10;
   break;
   case 29:
       dismem[3] = dismem[3]|0x20;
   break;
   case 30:
       dismem[3] = dismem[3]|0x40;
   break;
   case 31:
       dismem[3] = dismem[3]|0x80;
   break;
   case 32:
       dismem[2] = dismem[2]|0x08;
   break;
   case 33:
       dismem[2] = dismem[2]|0x04;
   break;
   case 34:
       dismem[2] = dismem[2]|0x02;
   break;
   case 35:
       dismem[2] = dismem[2]|0x01;
   break;
   case 36:
       dismem[2] = dismem[2]|0x10;
   break;
   case 37:
       dismem[2] = dismem[2]|0x20;
   break;
   case 38:
       dismem[2] = dismem[2]|0x40;
   break;
   case 39:
       dismem[2] = dismem[2]|0x80;
   break;
   case 40:
       dismem[1] = dismem[1]|0x08;
   break;
   case 41:
       dismem[1] = dismem[1]|0x04;
   break;
   case 42:
       dismem[1] = dismem[1]|0x02;
   break;
   case 43:
       dismem[1] = dismem[1]|0x01;
   break;
   case 44:
       dismem[1] = dismem[1]|0x10;
   break;
   case 45:
       dismem[1] = dismem[1]|0x20;
   break;
   case 46:
       dismem[1] = dismem[1]|0x40;
   break;
   case 47:
       dismem[1] = dismem[1]|0x80;
   break;
   case 48:
       dismem[0] = dismem[0]|0x08;
   break;
   case 49:
       dismem[0] = dismem[0]|0x04;
   break;
   case 50:
       dismem[0] = dismem[0]|0x02;
   break;
   case 51:
       dismem[0] = dismem[0]|0x20;
   break;
   default:
   break;
   }


}
void Time_Dis(unsigned char  min,unsigned char  second){

    unsigned char i,j,tmp;
   i= min >> 4;  
   j=Tablep[i];
   tmp = j>>4 ;
   dismem[0] = dismem[0]&0xf0;
   dismem[0] = dismem[0]|tmp; 
   tmp = j&0x0f ;
   tmp = tmp << 4;
   dismem[1] = dismem[1]&0x0f;
   dismem[1] = dismem[1]|tmp; //
   
   i= min & 0x0f;  
   j=Tablep[i];
   tmp = j>>4 ;
   dismem[1] = dismem[1]&0xf0;
   dismem[1] = dismem[1]|tmp; 
   tmp = j&0x0f ;
   tmp = tmp << 4;
   dismem[2] = dismem[2]&0x0f;
   dismem[2] = dismem[2]|tmp|0x80; //col����

   i= second >> 4;  
   j=Tablep[i];
   tmp = j>>4 ;
   dismem[2] = dismem[2]&0xf0;
   dismem[2] = dismem[2]|tmp; 
   tmp = j&0x0f ;
   tmp = tmp << 4;
   dismem[3] = dismem[3]&0x0f;
   dismem[3] = dismem[3]|tmp; //
   
   i= second & 0x0f;  
   j=Tablep[i];
   tmp = j>>4 ;
   dismem[3] = dismem[3]&0xf0;
   dismem[3] = dismem[3]|tmp; 
   tmp = j&0x0f ;
   tmp = tmp << 4;
   dismem[4] = dismem[4]&0x0f;
   dismem[4] = dismem[4]|tmp; 

}
void Temp_Dis(unsigned char n){
//extern unsigned char dismem[16];
    unsigned char i,j,tmp;
   i= n >> 4;  
   j=Tablep[i];
   if(n==0xFF)j=0;
   tmp = j>>4 ;
   dismem[4] = dismem[4]&0xf0;
   dismem[4] = dismem[4]|tmp; 
   tmp = j&0x0f ;
   tmp = tmp << 4;
   dismem[5] = dismem[5]&0x0f;
   dismem[5] = dismem[5]|tmp; //
   
   i= n & 0x0f;  
   j=Tablep[i];
   if(n==0xFF)j=0;
   tmp = j>>4 ;
   dismem[5] = dismem[5]&0xf0;
   dismem[5] = dismem[5]|tmp; 
   tmp = j&0x0f ;
   tmp = tmp << 4;
   dismem[6] = dismem[6]&0x0f;
   dismem[6] = dismem[6]|tmp; 
}

void Thengh_Dis(unsigned  char n){
//extern unsigned char dismem[16];
    unsigned char i,j,tmp;
   i= n >> 4;  
   j=Tablep[i];
   if(n==0xFF)j=0;
   tmp = j>>4 ;
   dismem[6] = dismem[6]&0xf0;
   dismem[6] = dismem[6]|tmp; 
   tmp = j&0x0f ;
   tmp = tmp << 4;
   dismem[7] = dismem[7]&0x0f;
   dismem[7] = dismem[7]|tmp; //
   
   i= n & 0x0f;  
   j=Tablep[i];
   if(n==0xFF)j=0;
   tmp = j>>4 ;
   dismem[7] = dismem[7]&0xf0;
   dismem[7] = dismem[7]|tmp; 
   tmp = j&0x0f ;
   tmp = tmp << 4;
   dismem[8] = dismem[8]&0x0f;
   dismem[8] = dismem[8]|tmp; 
}

void Dis_Clr(void){
//extern unsigned char dismem[16];
    char i;
    for(i=0;i<16;i++) dismem[i] = 0x00;
}
void T1_Dis(void){
//extern unsigned char dismem[16];
     dismem[0]=0xff;
}
////��ʾ�������
void Pane_Dis(unsigned char t){

  switch(t){
     case 0:
          dismem[8]=dismem[8]|0x0E;dismem[9]=dismem[9]|0xFF;break;  //ȫ��
     case 1:
          dismem[8]=dismem[8]&0xF1;dismem[9]=dismem[9]&0x00;
          dismem[9]=dismem[9]|0x08; break;
     case 2:
          dismem[8]=dismem[8]&0xF1;dismem[9]=dismem[9]&0x00;
          dismem[9]=dismem[9]|0x0C; break;
     case 3:
          dismem[8]=dismem[8]&0xF1;dismem[9]=dismem[9]&0x00;
          dismem[9]=dismem[9]|0x0E; break;
     case 4:
          dismem[8]=dismem[8]&0xF1;dismem[9]=dismem[9]&0x00;
          dismem[9]=dismem[9]|0x0F; break;
     case 5:
          dismem[8]=dismem[8]&0xF1;dismem[9]=dismem[9]&0x00;
          dismem[9]=dismem[9]|0x1F; break;
     case 6:
          dismem[8]=dismem[8]&0xF1;dismem[9]=dismem[9]&0x00;
          dismem[9]=dismem[9]|0x3F; break;
     case 7:
          dismem[8]=dismem[8]&0xF1;dismem[9]=dismem[9]&0x00;
          dismem[9]=dismem[9]|0x7F; break;
     case 8:
          dismem[8]=dismem[8]&0xF1;dismem[9]=dismem[9]&0x00;
          dismem[9]=dismem[9]|0xFF; break;
     case 9:
          dismem[8]=dismem[8]&0xF1;dismem[9]=dismem[9]&0x00;
          dismem[8]=dismem[8]|0x08; dismem[9]=dismem[9]|0xFF;break;
     case 10:
          dismem[8]=dismem[8]&0xF1;dismem[9]=dismem[9]&0x00;
          dismem[8]=dismem[8]|0x0C; dismem[9]=dismem[9]|0xFF;break;
     case 11:
          dismem[8]=dismem[8]&0xF1;dismem[9]=dismem[9]&0x00;
          dismem[8]=dismem[8]|0x0E;dismem[9]=dismem[9]|0xFF; break;
     default:
          dismem[8]=dismem[8]&0xF1;dismem[9]=dismem[9]&0x00;  //ȫ�� 
          break;
  }
}



// uint8_t To_Bcd(unsigned char t)   //2λ����תBCD
// { 
//   unsigned char out;
//   if(t==0xFF) out=t;
//   else  out=(t/10<<4)|(t%10); 
   
//    return out;
//  }

