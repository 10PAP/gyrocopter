#include <stm32f0xx.h>
#include "l3gd20.h"
#include "lamps.h"
#include "math.h"

//PB13 - SPI2_CLK
//PB15 - SPI2_MOSI
//PA8 - GPO_LE

#define DISPLAY_CS_HIGH (GPIOA->BSRR = GPIO_BSRR_BS_8)
#define DISPLAY_CS_LOW (GPIOA->BSRR = GPIO_BSRR_BR_8)

#define ODR (760) // Hz

#define M_PI (3.14159265358979323846)

static void Display_InitPins(void){
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER |= GPIO_MODER_MODER8_0;
	DISPLAY_CS_HIGH;
}

static void SPI_init(void){
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    SPI2->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR | SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA;
    SPI2->CR2 = (0x7UL << SPI_CR2_DS_Pos); // SPI_CR2_DS - 16 bit, now - 8 => QUESTION;
    SPI2->CR1 |= SPI_CR1_SPE;

    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    //GPIOB->AFR[0] |= Fn  << 4 * Pn; // Pn < 8
    //GPIOB->AFR[1] |= Fn << 4 * (Pn-8) // Pn >= 8
    GPIOB->AFR[1] |= (0 << 4 * (13 - 8)) | (0 << 4 * (15 - 8));
    GPIOB->MODER |= GPIO_MODER_MODER13_1 | GPIO_MODER_MODER15_1;

    /* enable mcu interrupts */
    NVIC_SetPriority(SPI2_IRQn, 64);
    NVIC_EnableIRQ(SPI2_IRQn);

    // enable spi2 interrupt: Rx buffer not empty interrupt enable
    SPI2->CR2 |= SPI_CR2_RXNEIE;
}


// business-logic map made of 8-bit columns
static uint8_t map[8];


// SPI interrupts
// enable interrupts from spi
// enable interrupts on MCU
// write handler

static void sendData(void)
{
    static uint8_t frameNumber = 0;
    GPIOA->BSRR = GPIO_BSRR_BR_8;

    // starting forming the data that will be send to spi
    uint16_t message = 0;
    message |= (1 << frameNumber);
    message |= (map[frameNumber] << 8);

    // finally - send that data
    SPI2->DR = message;

    frameNumber = (frameNumber + 1) & 7;
}

void SPI2_IRQHandler(void)
{
    /*** processing physical screen ***/
    // should we do LE?
    if (SPI2->SR & SPI_SR_RXNE)
    {
        // we have already sent the data: now we should clean up

        volatile uint16_t unusedVar = SPI2->DR;
        GPIOA->BSRR = GPIO_BSRR_BS_8; // LE = 1

        // should we draw new frame already?
        sendData();
    }
}

static void processData(L3GD20_Data_t* newData){
	static double angle = 0;
	static double h = 1.0 / ODR;
	static int8_t lastXIdx = 0;
	static int8_t lastYIdx = 0;
	
	map[lastXIdx] &= ~(1 << lastYIdx);
	
	angle += h * newData->Z;
	if(angle >= 360.0){
		angle = 0.0;
	}
	if(angle <= 0.0){
		angle = 360.0;
	}
	double angleRadians = (angle * 2 * M_PI / 360.0);
	double x = cos(angleRadians);
	double y = sin(angleRadians);
	int8_t xIdx = (int8_t)((x + 1.0) / 0.25);
	int8_t yIdx = (int8_t)((y + 1.0) / 0.25);
	xIdx = xIdx < 0 ? 0 : xIdx;
	yIdx = yIdx < 0 ? 0 : yIdx;
	xIdx = xIdx > 7 ? 7 : xIdx;
	yIdx = yIdx > 7 ? 7 : yIdx;
	map[xIdx] |= (1 << yIdx);
	lastXIdx = xIdx;
	lastYIdx = yIdx;
}

int main(void)
{
	Lamps_Init();
	
	Display_InitPins();
	
	L3GD20_InitPins();
	
	
	
	/* Init SPI QUESTION 16 bit??? */
	SPI_init();
    if(L3GD20_Init(L3GD20_Scale_250) == L3GD20_Result_Error){
		LedDownEnable;
	}
    
	sendData();

    // L3GD20 - scheme rev c
    // USART
    // rs-485
    while (1)
    {
		L3GD20_Status_t status = L3GD20_GetStatus();
		if(status == L3GD20_DataOverWritten){
			LedLeftEnable;
		}
		if(status == L3GD20_DataReady){
			// react
			L3GD20_Data_t newData;
			L3GD20_Read(&newData);
			processData(&newData);
		}
    }
}
