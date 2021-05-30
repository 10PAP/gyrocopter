#include <stm32f0xx.h>

#define L3GD20_CS_HIGH (GPIOC->BSRR = GPIO_BSRR_BS_0) ; L3GD20_bit = 1
#define L3GD20_CS_LOW (GPIOC->BSRR = GPIO_BSRR_BR_0) ; L3GD20_bit = 0

#define SPI_WAIT while(SPI2->SR & SPI_SR_BSY)

#define L3GD20_WHO_AM_I				0xD4

#define L3GD20_REG_WHO_AM_I			0x0F
#define L3GD20_REG_CTRL_REG1		0x20
#define L3GD20_REG_CTRL_REG2		0x21
#define L3GD20_REG_CTRL_REG3		0x22
#define L3GD20_REG_CTRL_REG4		0x23
#define L3GD20_REG_CTRL_REG5		0x24

#define L3GD20_REG_OUT_X_L			0x28
#define L3GD20_REG_OUT_X_H			0x29
#define L3GD20_REG_OUT_Y_L			0x2A
#define L3GD20_REG_OUT_Y_H			0x2B
#define L3GD20_REG_OUT_Z_L			0x2C
#define L3GD20_REG_OUT_Z_H			0x2D

#define L3GD20_REG_STATUS_REG		0x27
#define L3GD20_REG_REFERENCE		0x25

/* Sensitivity factors, datasheet pg. 9 */
#define L3GD20_SENSITIVITY_250		8.75	/* 8.75 mdps/digit */
#define L3GD20_SENSITIVITY_500		17.5	/* 17.5 mdps/digit */
#define L3GD20_SENSITIVITY_2000		70		/* 70 mdps/digit */

typedef enum {
	L3GD20_Result_Ok,
	L3GD20_Result_Error
} L3GD20_Result_t;

typedef enum {
	L3GD20_Scale_250, /*!< Set full scale to 250 mdps */
	L3GD20_Scale_500, /*!< Set full scale to 500 mdps */
	L3GD20_Scale_2000 /*!< Set full scale to 2000 mdps */
} L3GD20_Scale_t;

typedef struct {
	int16_t X; /*!< X axis rotation */
	int16_t Y; /*!< Y axis rotation */
	int16_t Z; /*!< Z axis rotation */
} L3GD20_Data_t;

typedef enum {
	L3GD20_DataNotReady,
	L3GD20_DataReady,
	L3GD20_DataOverWritten
} L3GD20_Status_t;

L3GD20_Result_t L3GD20_Init(L3GD20_Scale_t scale);
L3GD20_Result_t L3GD20_Read(L3GD20_Data_t* data);
L3GD20_Status_t L3GD20_GetStatus(void);
void L3GD20_InitPins(void);


