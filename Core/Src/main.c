#include "main.h"
#include "stm32f4xx.h"

void RCC_Init(void)
{
  // Enable HSI oscillator
  RCC->CR |= RCC_CR_HSION; // CR - Clock Control Register
  while (!(RCC->CR & RCC_CR_HSIRDY)); // Wait until HSI is ready

  FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_3WS; // Configure Flash Access Control Register for 100 MHz
  
  // Configure PLL: M=8, N=100, P=2, Source=HSI to achieve 100 MHz system clock
  RCC->PLLCFGR = (8 << RCC_PLLCFGR_PLLM_Pos) | 
                 (100 << RCC_PLLCFGR_PLLN_Pos) |
                 ( 0 << RCC_PLLCFGR_PLLP_Pos) |
                 RCC_PLLCFGR_PLLSRC_HSI;

  // Enable PLL
  RCC->CR |= RCC_CR_PLLON;
  while(!(RCC->CR & RCC_CR_PLLRDY)); // Wait until PLL is ready

  // Set AHB, APB1, APB2 prescalers
  RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;

  // Select PLL as system clock source
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while(!(RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SW_PLL); // Wait until PLL is used as system clock source
}

void Peripheral_Clock_Init(void)
{

  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // Enable clock for GPIOA
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN; // Enable clock for USART2
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; // Enable clock for ADC1

}
void GPIOA_Init(void)
{

  GPIOA->MODER |= (3 << 0); // Set PA0 to analog mode
  GPIOA->MODER |= (2 << (2 * 2)) | (2 << (2 * 3)); // Set PA2 and PA3 to alternate function mode
  GPIOA->AFR[0] |= (7 << (2 * 4)) | (7 << (3 * 4)); // Set alternate function for PA2 and PA3 to AF7 (USART2)

}

int main(void)
{
  RCC_Init(); // Initialize system clock to 100 MHz
  Peripheral_Clock_Init(); // Initialize peripheral clocks
  GPIOA_Init(); // Initialize GPIOA

  while (1)
  {

  }
}
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
 
}
#endif 
