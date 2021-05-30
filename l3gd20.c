#include "l3gd20.h"

uint8_t L3GD20_bit = 0;

/* private functions */

static uint8_t L3GD20_ReadSPI(uint8_t address);
static void L3GD20_WriteSPI(uint8_t address, uint8_t data);
static uint16_t SPI_Send(uint16_t data);

static L3GD20_Scale_t L3GD20_Scale;

L3GD20_Result_t L3GD20_Init(L3GD20_Scale_t scale){
	/* Vibe-check */
	if (L3GD20_ReadSPI(L3GD20_REG_WHO_AM_I) != L3GD20_WHO_AM_I) {
		/* Something went wrong */
		return L3GD20_Result_Error;
	}
	
	/* 1. Write CTRL_REG2 QUESTION */
	/* Set high-pass filter settings */
	L3GD20_WriteSPI(L3GD20_REG_CTRL_REG2, 0x20);
	//L3GD20_WriteSPI(L3GD20_REG_CTRL_REG2, 0x00);
	
	/* 2. Write CTRL_REG3 QUESTION */
	// L3GD20_WriteSPI(L3GD20_REG_CTRL_REG3, 0x08);
	
	/* 3. Write CTRL_REG4 */
	/* Set L3GD20 scale */
	if (scale == L3GD20_Scale_250) {
		L3GD20_WriteSPI(L3GD20_REG_CTRL_REG4, 0x00);
	} else if (scale == L3GD20_Scale_500) {
		L3GD20_WriteSPI(L3GD20_REG_CTRL_REG4, 0x10);
	} else if (scale == L3GD20_Scale_2000) {
		L3GD20_WriteSPI(L3GD20_REG_CTRL_REG4, 0x20);
	}
	
	/* Save scale */
	L3GD20_Scale = scale;
	
	
	/* 4. Write CTRL_REG6 QUESTION */
	
	/* 5. Write REFERENCE QUESTION*/
	L3GD20_ReadSPI(L3GD20_REG_REFERENCE);
	
	/* 6. Write INT1_THS QUESTION*/
	/* 7. Write INT1_DUR QUESTION*/
	/* 8. Write INT1_CFG QUESTION*/
	
	/* 9. Write CTRL_REG5 */
	/* Enable high-pass filter */
	L3GD20_WriteSPI(L3GD20_REG_CTRL_REG5, 0x10);
	
	/* 10. Write CTRL_REG1 */
	/* Enable L3GD20 Power bit - datasheet p.32 */
	L3GD20_WriteSPI(L3GD20_REG_CTRL_REG1, 0xCF/*0xFF*/);

	

	/* Everything OK */
	return L3GD20_Result_Ok;
}

L3GD20_Result_t L3GD20_Read(L3GD20_Data_t * data) {
	double temp, s;
	
	/* Read Z axis */
	data->Z = L3GD20_ReadSPI(L3GD20_REG_OUT_Z_L);
	data->Z |= L3GD20_ReadSPI(L3GD20_REG_OUT_Z_H) << 8;
	
	/* Set sensitivity scale correction */
	if (L3GD20_Scale == L3GD20_Scale_250) {
		/* Sensitivity at 250 range = 8.75 mdps/digit */
		s = L3GD20_SENSITIVITY_250 * 0.001;
	} else if (L3GD20_Scale == L3GD20_Scale_500) {
		/* Sensitivity at 500 range = 17.5 mdps/digit */
		s = L3GD20_SENSITIVITY_500 * 0.001;
	} else {
		/* Sensitivity at 2000 range = 70 mdps/digit */
		s = L3GD20_SENSITIVITY_2000 * 0.001;
	}
	
	temp = (double)data->X * s;
	data->X = (int16_t) temp;
	temp = (double)data->Y * s;
	data->Y = (int16_t) temp;
	temp = (double)data->Z * s;
	data->Z = (int16_t) temp;
	
	/* Return OK */
	return L3GD20_Result_Ok;
}


static uint16_t SPI_Send(uint16_t data) {
	SPI_WAIT;
	/* yesli zachelkivat' SE v displeye, to vrode danniye s gyroskopa tozge budut otpravlyat'sya */
	SPI2->DR = data;
	SPI_WAIT;
	return (uint16_t)SPI2->DR;
}
static uint8_t L3GD20_ReadSPI(uint8_t address) {
	uint16_t data;
	L3GD20_CS_LOW;
	data = SPI_Send((uint16_t)(((uint16_t)address | 0x80UL) << 8 | 0xFF));
	L3GD20_CS_HIGH;
	return (uint8_t)(data & 0xFF);
}

static void L3GD20_WriteSPI(uint8_t address, uint8_t data) {
	L3GD20_CS_LOW;
	SPI_Send((uint16_t)((uint16_t)data | ((uint16_t)address << 8)));
	L3GD20_CS_HIGH;
}

void L3GD20_InitPins(){
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	
	GPIOC->MODER |= GPIO_MODER_MODER0_0;
	GPIOC->PUPDR |= GPIO_PUPDR_PUPDR0_0;
	
	L3GD20_CS_HIGH;
}
L3GD20_Status_t L3GD20_GetStatus(void){
	L3GD20_Status_t data = L3GD20_ReadSPI(L3GD20_REG_STATUS_REG);
	
	if((data & (1UL << 3)) == 0){
		return L3GD20_DataNotReady;
	}
	if((data & (1UL << 7)) != 0){
		return L3GD20_DataOverWritten;
	}
	
	return data;
}
