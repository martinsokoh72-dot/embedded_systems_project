// Connect a POT betweek 0 and 3.3V with the wiper connected to PA0
// NOTE!!: PA0 actually connects to IN5 of ADC1.
// The POT voltage controls the duty cycle of a PWM output on PA3
// Timer 2 Channel 4     is used to generate the PWM output.
#include <stm32l432xx.h>
#include <stdint.h>
void setup(void);
void delay(volatile uint32_t dly);
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void initADC(void);
int readADC(int chan);
void initTimer2(void);
void setTimer2Duty(int duty);
void initClocks(void);
void delay_ms(unsigned dly);

// added from serial file
#include <eeng1030_lib.h>
#include <stdio.h>
#include  <errno.h>
#include  <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO

void initSerial(uint32_t baudrate);
void eputc(char c);

// define needed constants
#define pulse_per_rev 2

int vin, speed_hz, speed_rpm;
volatile unsigned pulse_count, milliseconds;

int main()
{
    setup();
    SysTick->LOAD = 80000-1; // Systick clock = 80MHz. 80000000/80000=1000
	SysTick->CTRL = 7; // enable systick counter and its interrupts
	SysTick->VAL = 10; // start from a low number so we don't wait for ages for first interrupt

    while(1)
    {        
        vin = readADC(5);  
        setTimer2Duty(vin);

        pulse_count = 0;

        delay_ms(1000);
    
        // evaluate speed 
        speed_hz = (pulse_count / pulse_per_rev);
        speed_rpm = (speed_hz * 60);
        // print fan speed 
        printf("For ADC: %d, the Fan speed (rpm) is %d\r\n", vin, speed_rpm);

    }
}

void setup(void)
{
    initClocks();
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // turn on GPIOA and GPIOB

    pinMode(GPIOB,3,1); // digital output
    pinMode(GPIOB,4,0); // digital input
    enablePullUp(GPIOB,4); // pull-up for button
    pinMode(GPIOA,0,3);  // analog input
    
    pinMode(GPIOA,3,2);  // alternative function mode
    GPIOA->AFR[0] &= ~(3 << (2*3));    // Clear out old alternative function  bits
    //GPIOA->AFR[0] &= ~(0xF << (3*4));
    GPIOA->AFR[0] |= 1 << (4*3);    // Select alternative function 1

    // serial communication
    initADC();
    initSerial(9600);
    initTimer2();

    // interrupt for counting pulses
    RCC->APB2ENR |= (1 << 0); // enable SYSCFG
    SYSCFG->EXTICR[1] &= ~(7 << 0); // clear perhaps previously set bits
    SYSCFG->EXTICR[1] |= (1 << 0);  // map EXTI2 interrupt to PB4
    EXTI->FTSR1 |= (1 << 4); // select falling edge trigger for PB4 input
    EXTI->IMR1 |= (1 << 4);  // enable PB4 interrupt
    NVIC->ISER[0] |= (1 << 10); // IRQ 10 maps to EXTI4
    __enable_irq();

}

void initADC()
{
    // initialize the ADC
    RCC->AHB2ENR |= (1 << 13); // enable the ADC
    RCC->CCIPR |= (1 << 29) | (1 << 28); // select system clock for ADC
    // set ADC clock = HCLK and turn on the voltage reference
    ADC1_COMMON->CCR = ((0b01) << 16) + (1 << 22) ; 
    // start ADC calibration    
    ADC1->CR=(1 << 28); // turn on the ADC voltage regulator and disable the ADC
    // wait for voltage regulator to stabilize 
    //(20 microseconds according to the datasheet).  
    //This gives about 180microseconds
    delay(100);
    ADC1->CR |= (1<< 31);
    while(ADC1->CR & (1 << 31)); // wait for calibration to finish.
    ADC1->CFGR = (1 << 31); // disable injection
    ADC1_COMMON->CCR |= (0x0f << 18);
}

int readADC(int chan)
{

    ADC1->SQR1 |= (chan << 6);
    ADC1->ISR = (1 << 3); // clear EOS flag
    ADC1->CR |= (1 << 0); // enable the ADC
    while ( (ADC1->ISR & (1 <<0))==0); // wait for ADC to be ready
    ADC1->CR |= (1 << 2); // start conversion
    while ( (ADC1->ISR & (1 <<3))==0); // wait for conversion to finish
    return ADC1->DR; // return the result
    ADC1->CR = 0;
}

void delay(volatile uint32_t dly)
{
    while(dly--);
}

void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber)
{
	Port->PUPDR = Port->PUPDR &~(3u << BitNumber*2); // clear pull-up resistor bits
	Port->PUPDR = Port->PUPDR | (1u << BitNumber*2); // set pull-up bit
}
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
	/*
        Modes : 00 = input
                01 = output
                10 = special function
                11 = analog mode
	*/
	uint32_t mode_value = Port->MODER;
	Mode = Mode << (2 * BitNumber);
	mode_value = mode_value & ~(3u << (BitNumber * 2));
	mode_value = mode_value | Mode;
	Port->MODER = mode_value;
}
void initTimer2(void)
{
    RCC->APB1ENR1 |= (1 << 0); // enable Timer 2
    TIM2->CR1 = 0;
    TIM2->CCMR2 = (0b110 << 12) + (1 << 11)+(1 << 10);
    TIM2->CCER |= (1 << 12);
    TIM2->ARR = 2000-1;
    TIM2->CCR4 = 500;
    TIM2->EGR |= (1 << 0);
    TIM2->CR1 = (1 << 7);
    TIM2->CR1 |= (1 << 0);  
}
void setTimer2Duty(int duty)
{
    int arrvalue=(duty*TIM2->ARR)/4095;
    TIM2->CCR4=arrvalue;
}
void initClocks()
{
	// Initialize the clock system to a higher speed.
	// At boot time, the clock is derived from the MSI clock 
	// which defaults to 4MHz.  Will set it to 80MHz
	// See chapter 6 of the reference manual (RM0393)
	    RCC->CR &= ~(1 << 24); // Make sure PLL is off

	    RCC->PLLCFGR = (1 << 25) + (1 << 24) + (1 << 22) + (1 << 21) + (1 << 17) + (80 << 8) + (1 << 0);	
	    RCC->CR |= (1 << 24); // Turn PLL on
	    while( (RCC->CR & (1 << 25))== 0); // Wait for PLL to be ready
	// configure flash for 4 wait states (required at 80MHz)
	    FLASH->ACR &= ~((1 << 2)+ (1 << 1) + (1 << 0));
	    FLASH->ACR |= (1 << 2); 
	    RCC->CFGR |= (1 << 1)+(1 << 0); // Select PLL as system clock
}

// added from serial communication file
void initSerial(uint32_t baudrate)
{
    RCC->AHB2ENR |= (1 << 0); // make sure GPIOA is turned on
    pinMode(GPIOA,2,2); // alternate function mode for PA2
    selectAlternateFunction(GPIOA,2,7); // AF7 = USART2 TX
    pinMode(GPIOA,15,2); 
    selectAlternateFunction(GPIOA,15,3);
    RCC->APB1ENR1 |= (1 << 17); // turn on USART2

	const uint32_t CLOCK_SPEED=80000000;    
	uint32_t BaudRateDivisor;
	
	BaudRateDivisor = CLOCK_SPEED/baudrate;	
	USART2->CR1 = 0;
	USART2->CR2 = 0;
	USART2->CR3 = (1 << 12); // disable over-run errors
	USART2->BRR = BaudRateDivisor;
	USART2->CR1 =  (1 << 3);  // enable the transmitter and receiver
    USART2->CR1 |=  (1 << 2);  // enable the transmitter and receiver
	USART2->CR1 |= (1 << 0);
}
int _write(int file, char *data, int len)
{
    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }
    while(len--)
    {
        eputc(*data);    
        data++;
    }    
    return 0;
}
void eputc(char c)
{
    while( (USART2->ISR & (1 << 6))==0); // wait for ongoing transmission to finish
    USART2->TDR=c;
}       

void EXTI4_IRQHandler()
{
    //PIOB->BSRR = (1 << 3); // set PB3 to turn on LED 
    //GPIOB->IDR |= (1 << 4);
    EXTI->PR1 = (1 << 4);   // clear interrupt pending flag
    pulse_count++; //counting the pulse
    
}
void SysTick_Handler(void)
{    
   milliseconds++;
}
void delay_ms(unsigned dly){
    unsigned end = milliseconds + dly;
    while(milliseconds != end);
}