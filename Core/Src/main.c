#include "main.h"
#include "stm32f4xx.h"
#include <stdio.h>

volatile uint8_t adc_flag = 0;

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

void USART2_Init(void);
void USART2_SendChar(char c);
void ADC1_Init(void);
uint16_t ADC1_Read(void);
void TIM5_Init(void);

int main(void)
{
  RCC_Init(); // Initialize system clock to 100 MHz
  Peripheral_Clock_Init(); // Initialize peripheral clocks
  GPIOA_Init(); // Initialize GPIOA

  USART2_Init(); // Initialize USART2
  ADC1_Init(); // Initialize ADC1

  while (1)
  {
    if (adc_flag) 
      {
          adc_flag = 0; // Скидаємо прапорець
          
          uint16_t pulse_val = ADC1_Read(); 
          sprintf(uart_buf, "%u\r\n", pulse_val); 
          USART2_SendString(uart_buf);
      }
  }
}

void USART2_Init(void)
{
  USART2->BRR = 0x01B2; // Set baud rate to 115200
  USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // Enable transmitter, receiver and USART
}

void USART2_SendString(char* str)
{
  while (*str)
  {
    USART2_SendChar(*str++);// Send each character in the string
  }
}

void ADC1_Init(void)
{
  ADC1->SMPR2 |= (4 << ADC_SMPR2_SMP0_Pos); // Set sample time
  ADC1->SQR1 &= ~ADC_SQR1_L;
  ADC1->SQR3 &= ~ADC_SQR3_SQ1;
  ADC1->CR2 |= ADC_CR2_ADON; // Enable ADC
}

uint16_t ADC1_Read(void)
{
  ADC1->CR2 |= ADC_CR2_SWSTART;// Start ADC conversion
  while(!(ADC1->SR & ADC_SR_EOC));
  return ADC1->DR; // Return ADC conversion result
}

void TIM5_Init(void)
{
  RCC->APB1ENR |= RCC_APB1ENR_TIM5EN; // Enable clock for TIM5


  TIM5->PSC = 100 - 1; // Set prescaler to 100 (assuming 100 MHz clock, this gives 1 MHz timer clock)
  TIM5->ARR = 2000 - 1; // Set auto-reload value to 2000 (this gives a timer overflow every 2 ms)

  TIM5->DIER |= TIM_DIER_UIE;
  
  NVIC_EnableIRQ(TIM5_IRQn) // Enable TIM5 interrupt in NVIC

  TIM5->CR1 |= TIM_CR1_CEN; // Start TIM5
}

void TIM5_IRQHandler(void)
{
  if (TIM5->SR & TIM_SR_UIF) 
  {
    TIM5->SR &= ~TIM_SR_UIF;  // Clear update interrupt flag
    adc_flag = 1;
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
