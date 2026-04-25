#include "AS5600.h"
//#include "arm_math.h"
#define abs(x) ((x)>0?(x):-(x))
#define _2PI 6.28318530718f


/********************* IIC Adapter *************************/
static int i2cWrite(uint8_t dev_addr, uint8_t *pData, uint32_t count) {
  int status;
  int i2c_time_out = I2C_TIME_OUT_BASE + count * I2C_TIME_OUT_BYTE; 
  status = HAL_I2C_Master_Transmit(&AS5600_I2C_HANDLE, (dev_addr<<1), pData, count, i2c_time_out);
	
	//status = softi2c_master_write(&g_softi2c, dev_addr, pData, count);

  return status;
}

static int i2cRead(uint8_t dev_addr, uint8_t *pData, uint32_t count) {
  int status;
  int i2c_time_out = I2C_TIME_OUT_BASE + count * I2C_TIME_OUT_BYTE;
  status = HAL_I2C_Master_Receive(&AS5600_I2C_HANDLE, (dev_addr<<1), pData, count, i2c_time_out);
	
	//status = softi2c_master_read(&g_softi2c, dev_addr, pData, count);
	
  return status;
}

static inline int i2c_Mem_Read(uint8_t dev_addr, uint8_t *pData, uint32_t count) {
  int status;
  int i2c_time_out = I2C_TIME_OUT_BASE + count * I2C_TIME_OUT_BYTE;
  status = HAL_I2C_Mem_Read(&AS5600_I2C_HANDLE, (dev_addr<<1), AS5600_RAW_ANGLE_REGISTER, I2C_MEMADD_SIZE_8BIT, pData, count, i2c_time_out);
	
	//status = softi2c_master_mem_read(&g_softi2c, dev_addr, AS5600_RAW_ANGLE_REGISTER, 1, pData, count);
	
  return status;
}


/******************* General Function **********************/
static inline int bsp_as5600GetRawAngle(uint16_t *raw_angle) {
	int ret;
	
  uint8_t buffer[2] = {0};
//  uint8_t raw_angle_register = AS5600_RAW_ANGLE_REGISTER;
//  ret = i2cWrite(AS5600_RAW_ADDR, &raw_angle_register, 1);
  ret = i2cRead(AS5600_RAW_ADDR, buffer, 2);
	
  *raw_angle = ((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1];
	
  return ret;
}

int  bsp_as5600GetAngle(volatile float *Angle) {
	int ret;
  uint16_t angle_data;
	ret =	bsp_as5600GetRawAngle(&angle_data);
  
	*Angle = (angle_data / (float)AS5600_RESOLUTION)*_2PI;
	
  return ret;
}

// Read STATUS register (1 byte)
int AS5600_ReadStatus(uint8_t *status)
{
	  int ret;
    uint8_t reg = AS5600_STATUS_REGISTER;
  
    ret = i2cWrite(AS5600_RAW_ADDR, ®, 1);
    if (ret != HAL_OK) return ret;

    ret = i2cRead(AS5600_RAW_ADDR, status, 1);
    return ret;
}

// Execute BURN_ANGLE (write 0x80 into 0xFF). Caller must ensure MD=1 and hardware power requirements.
// WARNING: irreversible and limited number of times. Check datasheet before using.
int AS5600_BurnAngle(void)
{
    uint8_t buf[2];
    int ret;

    buf[0] = AS5600_BURN_REGISTER;
    buf[1] = 0x80U; // BURN_ANGLE command

    ret = i2cWrite(AS5600_RAW_ADDR, buf, 2);
    if (ret != HAL_OK) return ret;

    // datasheet recommends waiting a bit and optionally verifying; wait small time
    HAL_Delay(5);
    return HAL_OK;
}


//配置AS5600的起始位置（ZPOS）
int bsp_AS5600_SetZero(uint8_t Burn)
{
	int ret;
	uint16_t RAWangle, ZPOS_RAWangle; 
	
	uint8_t buf[3];
	ret = bsp_as5600GetRawAngle(&RAWangle);
	if (ret != HAL_OK) return ret;
	uint8_t msb = (RAWangle >> 8) & 0x0F; // ZPOS(11:8)
  uint8_t lsb = RAWangle & 0xFF;        // ZPOS(7:0)
	
  buf[0] = AS5600_ZPOS11_8_REGISTER; // start register
	buf[1] = msb;
	buf[2] = lsb;
  ret = i2cWrite(AS5600_RAW_ADDR, buf, 3);
 	if (ret != HAL_OK) return ret;
  HAL_Delay(2);
	
	/******* Verify *******/
	uint8_t buffer[2] = {0};
  uint8_t raw_angle_register = AS5600_ZPOS11_8_REGISTER;
  ret = i2cWrite(AS5600_RAW_ADDR, &raw_angle_register, 1);
  ret = i2cRead(AS5600_RAW_ADDR, buffer, 2);
  ZPOS_RAWangle = ((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1];
	
	if((Burn == 1)&&(ret == HAL_OK)&&(ZPOS_RAWangle == RAWangle)){
		uint8_t status;
		if (AS5600_ReadStatus(&status) == HAL_OK) {
			 if (status & AS5600_STATUS_MD_MASK) {
				 // magnet detected -> allowed to burn
				 // double-check power, caps, magnet position before calling:
				 ret = AS5600_BurnAngle();
			 } 
			 else {
				 // magnet not detected: do not burn
			 }
	  }
  } 
	
  return ret;
}


/******************* User Function **********************/

Encoder_AS5600_t   Encoder_AS5600;

void bsp_as5600Init(void) {
  /* init i2c interface */
	uint8_t buffer[2] = {0};
	i2c_Mem_Read(AS5600_RAW_ADDR, buffer, 2);  //先定位到RAWangle寄存器，后续直接iic_read
		
  /* init var */
  Encoder_AS5600.rotation_circles = 0;
	bsp_as5600GetAngle(&Encoder_AS5600.old_angle);
	
    Encoder_AS5600.n_timeout = 4;
	Encoder_AS5600.dma_transfer_complete = true;
}


int bsp_as5600_DMAGetAngle(volatile float *Angle) {
	HAL_StatusTypeDef st;
	
	/*DMA 通讯*/
	if(Encoder_AS5600.dma_transfer_complete)
  {
    Encoder_AS5600.DMA_cnt = 0;
    Encoder_AS5600.dma_transfer_complete = false;
   
    st = HAL_I2C_Master_Receive_DMA(&AS5600_I2C_HANDLE, (AS5600_RAW_ADDR<<1), Encoder_AS5600.DMA_rxbuffer, 2);
	}
	else{
		Encoder_AS5600.DMA_cnt++;
	}
	
	/*DMA 超时判断*/
	if(Encoder_AS5600.DMA_cnt >= Encoder_AS5600.n_timeout){
		Encoder_AS5600.DMA_cnt = 0;
		// 简单复位 I2C 状态（依 HAL 版本而异）
		__HAL_I2C_DISABLE(&AS5600_I2C_HANDLE);
		__HAL_I2C_ENABLE(&AS5600_I2C_HANDLE);
		st = HAL_I2C_Master_Receive_DMA(&AS5600_I2C_HANDLE, (AS5600_RAW_ADDR<<1), Encoder_AS5600.DMA_rxbuffer, 2);
	}
	
	*Angle = Encoder_AS5600.angle;
	/*获取编码器角度*/
	return st;
}

 
// DMA 传输完成回调函数
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == &AS5600_I2C_HANDLE) // 确保是我们的 I²C 设备
    {
        uint16_t raw_angle = ((uint16_t)Encoder_AS5600.DMA_rxbuffer[0] << 8) | (uint16_t)Encoder_AS5600.DMA_rxbuffer[1];
	      Encoder_AS5600.angle = (raw_angle/(float)AS5600_RESOLUTION)*_2PI;
 
			  Encoder_AS5600.dma_transfer_complete = true;
    }
}
