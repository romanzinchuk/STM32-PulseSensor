#include "main.h"
#include "stm32f4xx.h"
#include <stdio.h>

volatile uint8_t adc_flag = 0;
float ema_alpha = 0.1f; 
float ema_value = 0.0f;

float dc_r = 0.95f; 
float dc_filter_output = 0.0f;
float prev_ema_value = 0.0f;

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
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL); // Wait until PLL is used as system clock source
}

void Peripheral_Clock_Init(void)
{

  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // Enable clock for GPIOA
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN; // Enable clock for USART2
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; // Enable clock for ADC1

}
void GPIOA_Init(void)
{

  GPIOA->MODER |= (3 << GPIO_MODER_MODER0_Pos); // PA0 as analog mode for ADC input
  GPIOA->MODER |= (2 << GPIO_MODER_MODER2_Pos) | (2 << GPIO_MODER_MODER3_Pos); // PA2 and PA3 as alternate function mode
  GPIOA->AFR[0] |= (7 << GPIO_AFRL_AFSEL2_Pos) | (7 << GPIO_AFRL_AFSEL3_Pos); // Set alternate function for PA2 and PA3 to AF7 (USART2)

}

void USART2_Init(void);
void USART2_SendChar(char c);
void USART2_SendString(char* str);
void ADC1_Init(void);
uint16_t ADC1_Read(void);
void TIM5_Init(void);

int main(void)
{
  SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2)); // Enable FPU (Floating Point Unit) for Cortex-M4

  RCC_Init(); // Initialize system clock to 100 MHz
  Peripheral_Clock_Init(); // Initialize peripheral clocks
  GPIOA_Init(); // Initialize GPIOA

  USART2_Init(); // Initialize USART2
  ADC1_Init(); // Initialize ADC1
  TIM5_Init();
  
  char uart_buf[50];
  while (1)
  {
      if (adc_flag) 
      {
          adc_flag = 0;
          uint16_t pulse_val = ADC1_Read(); 
        
          ema_value = (ema_alpha * (float)pulse_val) + ((1.0f - ema_alpha) * ema_value);
          dc_filter_output = ema_value - prev_ema_value + (dc_r * dc_filter_output);
          prev_ema_value = ema_value;

          int integer_part = (int)dc_filter_output;
          int fraction_part = (int)((dc_filter_output - integer_part) * 100);
          
          if (fraction_part < 0) 
          {
              fraction_part = -fraction_part;
              if (integer_part == 0 && dc_filter_output < 0) {
                  sprintf(uart_buf, "-0.%02d\r\n", fraction_part);
              } else {
                  sprintf(uart_buf, "%d.%02d\r\n", integer_part, fraction_part);
              }
          } 
          else 
          {
              sprintf(uart_buf, "%d.%02d\r\n", integer_part, fraction_part);
          }
          
          USART2_SendString(uart_buf);
      }
  }
}

void USART2_Init(void)
{
  USART2->BRR = 50000000 / 115200; // Set baud rate to 115200 (assuming 50 MHz APB1 clock)
  USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // Enable transmitter, receiver and USART
}

void USART2_SendChar(char c)
{
  while (!(USART2->SR & USART_SR_TXE)); // Wait until transmit data register is empty
  USART2->DR = c;
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
  
  NVIC_EnableIRQ(TIM5_IRQn); // Enable TIM5 interrupt in NVIC

  TIM5->CR1 |= TIM_CR1_CEN; // Start TIM5
}

void TIM5_IRQHandler(void)
{
  if (TIM5->SR & TIM_SR_UIF) 
  {
    TIM5->SR = ~TIM_SR_UIF;  // Clear update interrupt flag
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
