#include <stm32f0xx.h>
#include "lamps.h"
void Lamps_Init(void){
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC->MODER |= GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0;
}
