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

//MIDI numbers begin at 21, remember this when indexing

#define MAXNOTES 6 //arbitrarily chosen maximum number of notes
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
int numNotes = 6; //this WILL keep track of notes stored
int voiceNum = 2; //this will choose the voicing used
                  //CURRENTLY: 0=piano,1=clarinet,2=electric organ
//int amplitudes[MAXNOTES] = {1,0.7,0.5,0.4,0.3,0.2}; //used temporarily to create harmonics
void set_freq(float f[MAXNOTES]) {
    for(int i=0;i<MAXNOTES;i++){
        if(i < numNotes){
            step[i] = f[i]*N/RATE*(1<<16);}
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
        sample += wavetable[voiceNum][offset[i]>>16]; // *amplitudes[i];
                                            //used temporarily to create harmonics
    }
    sample = sample/16/numNotes; //implement variable to divide sample output
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
int main(void)
{
     setup_dac();
     //set sysTick to interrupt every 1ms
     init_SysTick(6000);
     init_tim7();
 //    int numNotes = 3;
     float notes[6] = {};
     for(;;){
         set_freq(notes);
     }
}
