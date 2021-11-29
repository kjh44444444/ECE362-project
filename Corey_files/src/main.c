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


#define MAXNOTES 8 //arbitrarily chosen maximum number of notes
#define NOTE_ON 0x90  //note on status message for channel 0
#define NOTE_OFF 0x80 //note off status message for channel 0
//int volume = 2048; //not currently used
int TimeDelay = 0;

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
     setup_dac();
     init_usart5();
     setup_dma();
     //set sysTick to interrupt every 1ms
//     init_SysTick(6000);
     init_tim7();
     for(;;){
//         set_freq(notes);
     }
}
