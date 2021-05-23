void Lamps_Init(void);
#define LedUpEnable (GPIOC->BSRR = GPIO_BSRR_BS_6)
#define LedUpDisable (GPIOC->BSRR = GPIO_BSRR_BR_6)

#define LedRightEnable (GPIOC->BSRR = GPIO_BSRR_BS_9)
#define LedRightDisable (GPIOC->BSRR = GPIO_BSRR_BR_9)

#define LedLeftEnable (GPIOC->BSRR = GPIO_BSRR_BS_8)
#define LedLeftDisable (GPIOC->BSRR = GPIO_BSRR_BR_8)

#define LedDownEnable (GPIOC->BSRR = GPIO_BSRR_BS_7)
#define LedDownDisable (GPIOC->BSRR = GPIO_BSRR_BR_7)
