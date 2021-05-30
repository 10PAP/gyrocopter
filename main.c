#include <stm32f0xx.h>
#include "l3gd20.h"
#include "lamps.h"
#include "math.h"
#include "usart.h"


//PB13 - SPI2_CLK
//PB15 - SPI2_MOSI
//PA8 - GPO_LE

static uint8_t display_bit = 0;
extern uint8_t L3GD20_bit;
#define DISPLAY_CS_HIGH (GPIOA->BSRR = GPIO_BSRR_BS_8) ; (display_bit = 1)
#define DISPLAY_CS_LOW (GPIOA->BSRR = GPIO_BSRR_BR_8) ; (display_bit = 0)

#define ODR (760) // Hz

#define M_PI (3.14159265358979323846)

#define DISPLAY_STATES_COUNTER 5

#define FIRST_BYTE (0xCC)
#define ACKNOWLEDGE (0xDD)


typedef enum USART_Role{
	USART_Sender,
	USART_Receiver
} USART_Role;

static USART_Role role;

static void SysTickInit()
{
    SystemCoreClockUpdate();
    SysTick->LOAD = SystemCoreClock / 100 - 1;
    SysTick->VAL = SystemCoreClock / 100 - 1;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
}

static void Display_InitPins(void){
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER |= GPIO_MODER_MODER8_0;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR8_0;
	DISPLAY_CS_HIGH;
}

static void SPI_init(void){
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    SPI2->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR | SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA;
    SPI2->CR2 =  SPI_CR2_DS; //(0x7UL << SPI_CR2_DS_Pos); // 
    SPI2->CR1 |= SPI_CR1_SPE;

    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    //GPIOB->AFR[0] |= Fn  << 4 * Pn; // Pn < 8
    //GPIOB->AFR[1] |= Fn << 4 * (Pn-8) // Pn >= 8
    GPIOB->AFR[1] |= (0 << 4 * (13 - 8)) | (0 << 4 * (15 - 8)) | (0 << 4 * (14 - 8));
    GPIOB->MODER |= GPIO_MODER_MODER13_1 | GPIO_MODER_MODER15_1 | GPIO_MODER_MODER14_1;
	
    
}


// business-logic map made of 8-bit columns
static uint8_t map[8];


static void sendData(void)
{
    static uint8_t frameNumber = 0;
    DISPLAY_CS_LOW;

    uint16_t message = 0;
    message |= (1 << frameNumber);
    message |= (map[frameNumber] << 8);

    SPI2->DR = message;
	
    frameNumber = (frameNumber + 1) & 7;
}

typedef enum SPI2_Handler_State {
	Display, L3GD20_Read1, L3GD20_Read2
} SPI2_Handler_State;

static L3GD20_Data_t gyro;
static uint8_t L3GD20_NewData_Ready = 0;
void SPI2_IRQHandler(void)
{
	static SPI2_Handler_State handler_state = Display;
	static uint8_t display_counter = 0;
	static volatile uint16_t unusedVar;
	if (SPI2->SR & SPI_SR_RXNE)
    {
		switch(handler_state){
			case Display:
				// what should we do when physical display received the data?
				unusedVar = SPI2->DR;
				DISPLAY_CS_HIGH;
				if (display_counter >= DISPLAY_STATES_COUNTER) {
					// send to gyro
					display_counter = 0;
					handler_state = L3GD20_Read1;
					L3GD20_CS_LOW;
					SPI2->DR = (uint16_t)(((uint16_t)L3GD20_REG_OUT_Z_L | 0x80UL) << 8 | 0xFF);
				}
				else{
					sendData();
				}
				display_counter++;
				break;
			case L3GD20_Read1:
				gyro.Z = (uint8_t)(((uint16_t)SPI2->DR) & 0xFF);
				L3GD20_CS_HIGH;
				handler_state = L3GD20_Read2;
				L3GD20_CS_LOW;
				SPI2->DR = (uint16_t)(((uint16_t)L3GD20_REG_OUT_Z_H | 0x80UL) << 8 | 0xFF);
				break;
			case L3GD20_Read2:
				gyro.Z |= ((uint8_t)(((uint16_t)SPI2->DR) & 0xFF)) << 8;
				L3GD20_CS_HIGH;
				handler_state = Display;
				L3GD20_NewData_Ready = 1;
				sendData();
				break;
		}
	}
	/*
	static int counter = 0;
	counter++;
	if(counter % 100 == 0){
		LedRightEnable;
		LedLeftDisable;
	}
	if(counter % 100 == 50){
		LedLeftEnable;
		LedRightDisable;
	}
	*/
}

#define abs(x) (x >= 0 ? x : -x)


void drawLine(int x0, int y0, int x1, int y1)
{
	int A, B, sign;
	A = y1 - y0;
	B = x0 - x1;
	if (abs(A) > abs(B)) sign = 1;
	else sign = -1;
	int signa, signb;
	if (A < 0) signa = -1;
	else signa = 1;
	if (B < 0) signb = -1;
	else signb = 1;
	int f = 0;
	map[x0] |= 1 << y0;
	int x = x0, y = y0;
	if (sign == -1) 
	{
	do {
	  f += A*signa;
	  if (f > 0)
	  {
		f -= B*signb;
		y += signa;
	  }
	  x -= signb;
	  map[x] |= 1 << y;
	} while (x != x1 || y != y1);
	}
	else
	{
	do {
	  f += B*signb;
	  if (f > 0) {
		f -= A*signa;
		x -= signb;
	  }
	  y += signa;
	  map[x] |= 1 << y;
	} while (x != x1 || y != y1);
	}
}


static uint8_t processData(L3GD20_Data_t* newData){
	static double h = 1.0 / ODR;
	static double angle = 0;
	
	angle += h * newData->Z / (3 * M_PI);
	if(angle >= 360.0){
		angle -= 360.0;
	}
	if(angle <= 0.0){
		angle += 360.0;
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
	
	
	
	return (uint8_t)((xIdx | (yIdx << 4)));
	
	
	
	/*
	LedDownDisable;
	LedRightDisable;
	LedUpDisable;
	LedLeftDisable;
	if(angle > 0 && angle <= 90){
		LedRightEnable;
	}
	if(angle > 90 && angle <= 180){
		LedUpEnable;
	}
	if(angle > 180 && angle <= 270){
		LedLeftEnable;
	}
	if(angle > 270 && angle <= 360){
		LedDownEnable;
	}
	*/
	
}

static void kernel_panic(){
	LedRightEnable;
	LedUpEnable;
	LedLeftEnable;
	LedDownEnable;
	for(int i = 0 ; i < 8 ; i++){
		map[i] = 0xFF;
	}
}

void SysTick_Handler(void){
	
}




static void become_sender(){
	role = USART_Sender;
}
static void become_receiver(){
	role = USART_Receiver;
}

static uint8_t newExternalData = 0;
static uint8_t USART_Data = 0;


void USART3_4_IRQHandler(){
	uint32_t status = USART3->ISR;
	if((status & USART_ISR_RXNE) != 0){
		newExternalData = 1;
		USART_Data = (uint8_t)(USART3->RDR);
		become_sender();
		USART3->TDR = processData(&gyro);
	}
	else if((status & USART_ISR_TC) != 0){
		USART3->ICR |= USART_ICR_TCCF | USART_ICR_IDLECF;
		become_receiver();
	}
}

int main(void)
{
	Lamps_Init();
	SysTickInit();
	Display_InitPins();
	
	L3GD20_InitPins();
	
	SPI_init();
	
	
	
    if(L3GD20_Init(L3GD20_Scale_250) == L3GD20_Result_Error){
		kernel_panic();
	}
	
	/* enable mcu interrupts */
    NVIC_SetPriority(SPI2_IRQn, 64);
    NVIC_EnableIRQ(SPI2_IRQn);
    SPI2->CR2 |= SPI_CR2_RXNEIE;
	
	sendData();

	USART_Init();
	
	
	USART3->CR1 |= USART_CR1_TE;
	for(int i = 0 ; i < 1000000 ; i++);
	USART3->TDR = FIRST_BYTE;
	while((USART3->ISR & USART_ISR_TC) == 0);
	USART3->ICR |= USART_ICR_TCCF | USART_ICR_IDLECF;
	become_receiver();
	
	// time-out
	for(int i = 0 ; i < 100000 ; i++);
	if((USART3->ISR & USART_ISR_RXNE) == 0){
		// we are the receiver
		while((USART3->ISR & USART_ISR_RXNE) == 0)
		{
			// waiting for enabling of the second mcu
		}
		uint8_t byte = (uint8_t)(USART3->RDR);
		if(byte != FIRST_BYTE){
			kernel_panic();
		}
		else{
			become_sender();
			USART3->TDR = ACKNOWLEDGE;
			while((USART3->ISR & USART_ISR_TC) == 0);
			USART3->ICR |= USART_ICR_TCCF | USART_ICR_IDLECF;
			become_receiver();
		}
	}
	else{
		uint8_t acknowledgement = (uint8_t)(USART3->RDR);
		if(acknowledgement == ACKNOWLEDGE){
			become_sender();
		}
	}
	
	/*
	if(role == USART_Sender){
		map[0] = 0xF0;
		
	}
	if(role == USART_Receiver){
		map[1] = 0x0F;
	}
	*/
	USART3->CR1 |= USART_CR1_RXNEIE | USART_CR1_TCIE;
	NVIC_EnableIRQ(USART3_4_IRQn);
	if(role == USART_Sender){
		uint8_t toSend = processData(&gyro);
		USART3->TDR = toSend;
	}
    while (1)
    {
		if(newExternalData){
			uint8_t xIdx = USART_Data & 0x0F;
			uint8_t yIdx = USART_Data >> 4;
			
			uint8_t centerX, centerY;
			if(xIdx >= 4)
				centerX = 4;
			else
				centerX = 3;
			if(yIdx >= 4)
				centerY = 4;
			else
				centerY = 3;
			for(int i = 0 ; i < 8 ; i++){
				map[i] = 0;
			}
			drawLine(centerX, centerY, xIdx, yIdx);
			newExternalData = 0;
		}
    }
}
