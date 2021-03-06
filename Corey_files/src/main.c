/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f0xx.h"
#include "constants.h"
#include "wavetable.h"
#include "MidiArray.h"
#include "stdio.h"
#include "stdlib.h"

#define MAXNOTES 8 //arbitrarily chosen maximum number of notes
#define NOTE_ON 0x90  //note on status message for channel 0
#define NOTE_OFF 0x80 //note off status message for channel 0
//int volume = 2048; //not currently used
int TimeDelay = 0;
int current_position = 0;
int position = 0;
int step[MAXNOTES];
int offset[MAXNOTES];

int numNotes = 0; //keeps track of number of notes stored
int voiceNum = 0; //this will choose the voicing used
                  //CURRENTLY: 0=piano,1=clarinet,2=electric organ...
int notesMIDI[MAXNOTES] = {}; //stores currently pressed notes (using MIDI number)
int noteIndex[127] = {}; //this keeps track of the index of a given note in
                         //notesMIDI (i.e. if Note ON-60 message received
                         //noteIndex[60] will be set to the index of notesMIDI
                         //where is stored)
//code for oled
void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}
void enable_ports(void){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER &= ~GPIO_MODER_MODER0 & ~GPIO_MODER_MODER2 & ~GPIO_MODER_MODER4;
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR0 & ~GPIO_PUPDR_PUPDR2 & ~GPIO_PUPDR_PUPDR4;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR0_1 | GPIO_PUPDR_PUPDR2_1 | GPIO_PUPDR_PUPDR4_1;
}
void init_exti(void){//using exti
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;
    SYSCFG->EXTICR[1] |= SYSCFG_EXTICR1_EXTI0_PA | SYSCFG_EXTICR1_EXTI2_PA;
    SYSCFG->EXTICR[2] |= SYSCFG_EXTICR2_EXTI4_PA;
    EXTI->RTSR |= EXTI_RTSR_TR0 | EXTI_RTSR_TR2 | EXTI_RTSR_TR4;
    EXTI->IMR |= EXTI_IMR_MR0 | EXTI_IMR_MR2 | EXTI_IMR_MR4;
    NVIC->ISER[0] |= 1<<EXTI0_1_IRQn | 1<<EXTI2_3_IRQn | 1<<EXTI4_15_IRQn;
}
void EXTI0_1_IRQHandler(void){
    position++;
    keypad(current_position,1);
    keypad(position,2);
    EXTI->PR = 0x1;
}
void EXTI2_3_IRQHandler(void){
    position--;
    keypad(current_position,1);
    keypad(position,2);
    EXTI->PR = 0x4;
}
void EXTI4_15_IRQHandler(void){
    current_position=position;
    keypad(current_position,1);
    keypad(position,2);
    EXTI->PR = 0x10;
}
void setup_spi1() {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER &= ~GPIO_MODER_MODER5 & ~GPIO_MODER_MODER6 & ~GPIO_MODER_MODER7 & ~GPIO_MODER_MODER15;
    GPIOA->MODER |= GPIO_MODER_MODER5_1 | GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1 | GPIO_MODER_MODER15_1;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    SPI1->CR1 &= ~SPI_CR1_SPE; //Ensure that the CR1_SPE bit is clear.
    SPI1->CR1 |= SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2; //Set the baud rate as low as possible (maximum divisor for BR).
    SPI1->CR2 = SPI_CR2_DS_3 | SPI_CR2_DS_0;
    SPI1->CR1 |= SPI_CR1_MSTR;
    SPI1->CR2 |= SPI_CR2_SSOE;
    SPI1->CR2 |= SPI_CR2_NSSP;
    SPI1->CR1 |= SPI_CR1_SPE;
}
void spi_cmd(unsigned int data) {
    while(!(SPI1->SR  & SPI_SR_TXE)); //Waits until the SPI1_SR_TXE bit is set.
    SPI1->DR = data; //Copies the parameter to the SPI1_DR.
}
void spi_data(unsigned int data) {
    while(!(SPI1->SR  & SPI_SR_TXE)); //Waits until the SPI1_SR_TXE bit is set.
    data |= 0x200;
    SPI1->DR = data;//Copies the parameter to the SPI1_DR.
}
void spi_init_oled() {
    nano_wait(1000000);//Use nano_wait() to wait 1 ms for the display to power up and stabilize.
    spi_cmd(0x38); // set for 8-bit operation
    spi_cmd(0x08); // turn display off
    spi_cmd(0x01); // clear display
    nano_wait(2000000);//Use nano_wait() to wait 2 ms for the display to clear.
    spi_cmd(0x06); // set the display to scroll
    spi_cmd(0x02); // move the cursor to the home position
    spi_cmd(0x0c); // turn the display on
}
void spi_display1(const char *string) {
    spi_cmd(0x02);
    for(int i =0 ;i<strlen(string);i++){
        if(string[i] != NULL){
            spi_data(string[i]);
        }
    }
}
void spi_display2(const char *string) {
    spi_cmd(0xc0);
    for(int i =0 ;i<strlen(string);i++){
        if(string[i] != NULL){
            spi_data(string[i]);
        }
    }
}
void keypad(int position,int keypad){
    if(keypad == 1){
        switch(position){
            case 0:spi_display1("                ");spi_display1("0: Grand piano");break;
            case 1:spi_display1("                ");spi_display1("1. Piano");break;
            case 2:spi_display1("                ");spi_display1("2. Organ");break;
            case 3:spi_display1("                ");spi_display1("3. Guitar");break;
            case 4:spi_display1("                ");spi_display1("4. Brass");break;
            case 5:spi_display1("                ");spi_display1("5. Saxophone");break;
            case 6:spi_display1("                ");spi_display1("6. Flute");break;
            case 7:spi_display1("                ");spi_display1("7. Strings");break;
            case 8:spi_display1("                ");spi_display1("8. Flute");break;
            case 9:spi_display1("                ");spi_display1("9. Synth");break;
            default: spi_display1("                ");
        }
    }else if(keypad == 2){
        switch(position){
            case 0:spi_display2("                ");spi_display2("0: Grand piano");break;
            case 1:spi_display2("                ");spi_display2("1. Piano");break;
            case 2:spi_display2("                ");spi_display2("2. Organ");break;
            case 3:spi_display2("                ");spi_display2("3. Guitar");break;
            case 4:spi_display2("                ");spi_display2("4. Brass");break;
            case 5:spi_display2("                ");spi_display2("5. Saxophone");break;
            case 6:spi_display2("                ");spi_display2("6. Flute");break;
            case 7:spi_display2("                ");spi_display2("7. Strings");break;
            case 8:spi_display2("                ");spi_display2("8. Flute");break;
            case 9:spi_display2("                ");spi_display2("9. Synth");break;
            default: spi_display2("                ");
        }
    }
}

//**
// The following code is used for delay between changing notes
// the function delay sets the global variable TimeDelay to the
// input nTime value and waits until it is decremented to 0 by
// the SysTick exception handler
//**
void init_SysTick(uint32_t ticks){
    //Disable IRQ and counter
    SysTick->CTRL = 0;
    //set reload register
    SysTick->LOAD = ticks - 1;
    //(this makes systick least urgent)
    NVIC_SetPriority (SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1);
    //Reset the counter value
    SysTick->VAL = 0;
    //Select clock (1 = processor clock 48MHz, 0 = external clock 6MHz)
    //6MHz chosen
    SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE_Msk;
    //enable exception request
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    //Enable SysTick
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

}
void SysTick_Handler (void) {
    if (TimeDelay > 0)
        TimeDelay --;
}
void delay(uint32_t nTime) {
    TimeDelay = nTime;
    while(TimeDelay != 0);
}
//**
//The following code sets up the dac
//**
void setup_dac() {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER |= 3<<(2*4);
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR |= DAC_CR_TEN1;
    DAC->CR |= DAC_CR_TSEL1;
    DAC->CR |= DAC_CR_EN1;
    return;

}
//**
// setup tim7 for DAC update
//**
void init_tim7(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7->PSC = 48 - 1;
    int pscSpd = 1000000;
    int arrVal = pscSpd/RATE;
    TIM7->ARR = arrVal - 1;
    TIM7->DIER |= TIM_DIER_UIE;
    TIM7->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 1 << TIM7_IRQn;
}

void set_freq(void) {
    float freq;
    for(int i=0;i<MAXNOTES;i++){
        if(i < numNotes){
            freq = midiArray[notesMIDI[i]];
            step[i] = freq*N/RATE*(1<<16);}
        else{
            step[i] = 0;
        }
    }
    return;
}

void TIM7_IRQHandler(void) {
    TIM7->SR &= ~TIM_SR_UIF;
    DAC->SWTRIGR |= DAC_SWTRIGR_SWTRIG1;
    int i;
    int sample = 0;
    for(i=0;i<MAXNOTES;i++){
        offset[i] += step[i];

        if(offset[i]>>16 >=N)
            offset[i] -= N<<16;
        sample += wavetable[voiceNum][offset[i]>>16];
    }

    int numNotesDiv;
    //For zero-division protection
    if(numNotes == 0){
        numNotesDiv = 1;
    }
    else{
        numNotesDiv = numNotes;
    }

    sample = sample/16/numNotesDiv; //implement variable to divide sample output
                                    //each time new note added
                                    //see notes to implement *volume* here
    sample += 2048;
    // clipping to prevent overflow
    if(sample > 4095)
        sample = 4095;
    else if (sample < 0)
        sample = 0;
    DAC->DHR12R1 = sample;
    return;

}
//**
//Original code for arpeggio design
//**
void arpeggio (float* notes, int numNotes, int delayTime) {
    for(int i = 0;i<numNotes;i++){
//        set_freq(notes[i]);
        delay(delayTime);
    }
}
void reverse_arpeggio (float* notes, int numNotes, int delayTime) {
    for(int i = numNotes-1;i>=0;i--){
//        set_freq(notes[i]);
        delay(delayTime);
    }
}

//initialize USART for receiving MIDI
void init_usart5() {
    RCC->AHBENR |= RCC_AHBENR_GPIODEN;

    GPIOD->MODER &= ~GPIO_MODER_MODER2;
    GPIOD->MODER |= GPIO_MODER_MODER2_1;
    GPIOD->AFR[0] &= ~GPIO_AFRL_AFR2;
    GPIOD-> AFR[0] |= 0x2 << (4*(2));

    RCC->APB1ENR |= RCC_APB1ENR_USART5EN;
    USART5->CR1 &= ~USART_CR1_UE;
    USART5->CR1 &= ~(USART_CR1_M | 0x10000000);
    USART5->CR2 &= ~USART_CR2_STOP;
    USART5->CR1 &= ~USART_CR1_PCE;
    USART5->CR1 &= ~USART_CR1_OVER8;
    USART5->BRR = 1536; //set baud rate to 31250
    USART5->CR1 |= USART_CR1_RE; //enable  receiver
    USART5->CR1 |= USART_CR1_UE;
    while((USART5->ISR & (USART_ISR_REACK | USART_ISR_REACK)) != (USART_ISR_REACK | USART_ISR_REACK));

}
//**
//Set up DMA for USART transfer
//**
#define MESSAGESIZE 3
uint8_t MIDImsg[MESSAGESIZE]; //stores the 3 byte MIDI message
int msgoffset = 0;
void setup_dma() {
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    USART5->CR3 |= USART_CR3_DMAR;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1->RMPCR |= DMA_RMPCR1_CH1_USART5_RX;
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    //configure DMA
    DMA1_Channel1->CMAR = (uint32_t) &MIDImsg; //may not need cast
    DMA1_Channel1->CPAR = (uint32_t) &(USART5->RDR);
    DMA1_Channel1->CNDTR = MESSAGESIZE;
    DMA1_Channel1->CCR &= ~DMA_CCR_DIR;
    DMA1_Channel1->CCR |= DMA_CCR_TCIE; //enable transfer complete interrupt
    DMA1_Channel1->CCR &= ~DMA_CCR_HTIE;
    DMA1_Channel1->CCR &= ~(DMA_CCR_MSIZE | DMA_CCR_PSIZE);
    DMA1_Channel1->CCR |= DMA_CCR_MINC;
    DMA1_Channel1->CCR &= ~DMA_CCR_PINC;
    DMA1_Channel1->CCR |= DMA_CCR_CIRC;
    DMA1_Channel1->CCR &= ~DMA_CCR_MEM2MEM;

    NVIC_SetPriority(DMA1_Channel1_IRQn,0);
    DMA1_Channel1->CCR |= DMA_CCR_EN;
    return;
}
//**
//Logic used to add or take away notes from notesMIDI array
//**
void noteOn(int notenumber){
    //to prevent overflow
    if(numNotes < MAXNOTES){
        notesMIDI[numNotes] = notenumber;
        noteIndex[notenumber] = numNotes;
        numNotes += 1;
    }
    set_freq();
    return;
}
void noteOff(int notenumber){
    int index = noteIndex[notenumber];
    //replace offnote with the last non-zero note in array
    //and set last non-zero note to 0
    if(numNotes > 0){
        notesMIDI[index] = notesMIDI[numNotes-1];
        notesMIDI[numNotes-1] = 0;
        numNotes -= 1;
    }
    set_freq();
    return;
}
void DMA1_CH1_IRQHandler(void) {
    DMA1->IFCR |= DMA_IFCR_CTCIF1;
    if(MIDImsg[0] == NOTE_ON){
        noteOn(MIDImsg[1]);
    }
    else if(MIDImsg[0] == NOTE_OFF){
        noteOff(MIDImsg[1]);
    }
    //add velocity statements later
    return;
}

int main(void)
{
     //setup oled
     enable_ports();
     init_exti();
     setup_spi1();
     spi_init_oled();
     keypad(current_position,1);
     keypad(position,2);
     setup_dac();
     init_usart5();
     setup_dma();
     //set sysTick to interrupt every 1ms
//     init_SysTick(6000);
     init_tim7();
     for(;;){
         set_freq(notes);
     }
}
