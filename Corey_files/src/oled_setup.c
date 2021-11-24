#include "stm32f0xx.h"
#include "stdio.h"
#include "stdlib.h"

const char keymap[] = "DCBA#9630852*741";
uint8_t hist[16];
uint8_t col;
char queue[2];
int qin;
int qout;

void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void enable_ports(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOB->MODER &= ~0x3fffff;
    GPIOB->MODER |= 0x155555;
    GPIOC->MODER &= ~0xffff;
    GPIOC->MODER |= 0x5500;
    GPIOC->PUPDR &= ~0xff;
    GPIOC->PUPDR |= 0xaa;
}

void drive_column(int c) {
    GPIOC->BSRR = 0xf00000 | (1 << (c + 4));
}

int read_rows() {
    return GPIOC->IDR & 0xf;
}

void push_queue(int n) {
    n = (n & 0xff) | 0x80;
    queue[qin] = n;
    qin ^= 1;
}

uint8_t pop_queue() {
    uint8_t tmp = queue[qout] & 0x7f;
    queue[qout] = 0;
    qout ^= 1;
    return tmp;
}

void update_history(int c, int rows) {
    for(int i = 0; i < 4; i++) {
        hist[4*c+i] = (hist[4*c+i]<<1) + ((rows>>i)&1);
        if (hist[4*c+i] == 1)
        push_queue(4*c+i);
    }
}

void init_tim6() {
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->PSC = 48 - 1;
    TIM6->ARR = 1000 - 1;
    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 1 << TIM6_DAC_IRQn;
}

void TIM6_DAC_IRQHandler(void) {
    TIM6->SR &= ~TIM_SR_UIF;
    int rows = read_rows();
    update_history(col, rows);
    col = (col + 1) & 3;
    drive_column(col);
}

char get_keypress() {
    for(;;) {
        asm volatile ("wfi" : :);   // wait for an interrupt
        if (queue[qout] == 0)
            continue;
        return keymap[pop_queue()];
    }
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

int main(void)
{
    int key = 0;
    int position = 0;
    int current_position = 0;

    enable_ports();
    init_tim6();
    setup_spi1();
    spi_init_oled();
    keypad1(current_position);
    keypad2(position);
    for(;;) {
        char key = get_keypress();
        switch(key){
            case '0':position = 0;keypad2(position);break;
            case '1':position = 1;keypad2(position);break;
            case '2':position = 2;keypad2(position);break;
            case '3':position = 3;keypad2(position);break;
            case '4':position = 4;keypad2(position);break;
            case '5':position = 5;keypad2(position);break;
            case '6':position = 6;keypad2(position);break;
            case '7':position = 7;keypad2(position);break;
            case '8':position = 8;keypad2(position);break;
            case '9':position = 9;keypad2(position);break;
            case 'A': if(position < 9) keypad2(++position);break;
            case 'B': if(position > 0) keypad2(--position);break;
            case '#': current_position = position;keypad1(current_position);break;
            default: spi_display2("                ");
        }
    }
}
void keypad1(int current_position){
    for(int i = 0;i<10;i++){
        if(i==current_position){
            char key = '0' + current_position;
            switch(key){
                case '0':spi_display1("                ");spi_display1("0: Grand piano");current_position = 0;break;
                case '1':spi_display1("                ");spi_display1("1. Piano");current_position = 1;break;
                case '2':spi_display1("                ");spi_display1("2. Organ");current_position = 2;break;
                case '3':spi_display1("                ");spi_display1("3. Guitar");current_position = 3;break;
                case '4':spi_display1("                ");spi_display1("4. Brass");current_position = 4;break;
                case '5':spi_display1("                ");spi_display1("5. Saxophone");current_position = 5;break;
                case '6':spi_display1("                ");spi_display1("6. Flute");current_position = 6;break;
                case '7':spi_display1("                ");spi_display1("7. Strings");current_position = 7;break;
                case '8':spi_display1("                ");spi_display1("8. Flute");current_position = 8;break;
                case '9':spi_display1("                ");spi_display1("9. Synth");current_position = 9;break;
                default: spi_display1("                ");
            }
        }
    }

}
void keypad2(int position){
    char key = '0' + position;
    switch(key){
        case '0':spi_display2("                ");spi_display2("0: Grand piano");position = 0;break;
        case '1':spi_display2("                ");spi_display2("1. Piano");position = 1;break;
        case '2':spi_display2("                ");spi_display2("2. Organ");position = 2;break;
        case '3':spi_display2("                ");spi_display2("3. Guitar");position = 3;break;
        case '4':spi_display2("                ");spi_display2("4. Brass");position = 4;break;
        case '5':spi_display2("                ");spi_display2("5. Saxophone");position = 5;break;
        case '6':spi_display2("                ");spi_display2("6. Flute");position = 6;break;
        case '7':spi_display2("                ");spi_display2("7. Strings");position = 7;break;
        case '8':spi_display2("                ");spi_display2("8. Flute");position = 8;break;
        case '9':spi_display2("                ");spi_display2("9. Synth");position = 9;break;
        default: spi_display2("                ");
    }
}
