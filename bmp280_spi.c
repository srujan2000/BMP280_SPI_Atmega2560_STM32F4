#define F_CPU 16000000UL

#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#define SPC_R *((volatile uint8_t*)0x4C) //control register
#define SPS_R *((volatile uint8_t*)0x4D) //status  register
#define SPD_R *((volatile uint8_t*)0x4E) //data    register

#define DDRB_R  *((volatile uint8_t*)0x24)  //data direction of port b 
#define PORTB_R *((volatile uint8_t*)0x25)  //data out of port b 


#define TCCR1A_R *((volatile uint8_t*)0x80)  //timer_1 control register A
#define TCCR1B_R *((volatile uint8_t*)0x81)  //timer_1 control register B
#define TCNT1_R  *((volatile uint16_t*)0x84) //timer_1  counter
#define OCR1A_R  *((volatile uint16_t*)0x88) //timer_1 output compare register A
#define TIMSK1_R  *((volatile uint8_t*)0x6F) //timer_1 interrupt mask register


#define UCSR0_A *((volatile uint8_t*)0xC0) // UART control and status register A
#define UCSR0_B *((volatile uint8_t*)0xC1) // UART control and status register B
#define UCSR0_C *((volatile uint8_t*)0xC2) // UART control and status register C
#define UBBR_0  *((volatile uint8_t*)0xC4)   //Baud rate
#define UDR_0   *((volatile uint8_t*)0xC6)  //data register

void timer_init(void);
void spi_init(void);
void uart_init(void);
void uart_tx(uint8_t);
uint8_t spi_trans(uint8_t);
void get_temp(void);
long temp_calc(long);
void print_values(void);

uint8_t data_buffer[10];

int main(void)
{	
	spi_init();
	uart_init();
	
	
	PORTB_R &= ~(0x01);
	spi_trans(0xF4 & 0x7F);//Mode and sampling register ,Normal Mode ,16x sampling
	spi_trans(0xA3);
	spi_trans(0xF5 & 0x7F);//config register,standby_mode-500 and filter and NO SPI
	spi_trans(0x8C);
	PORTB_R |= (0x01);
	
	timer_init();
	
    while (1) 
    {
		//get_temp();
		//print_values();
		//delay1();
		
    }
}


void timer_init(){
	
	TCCR1A_R = 0x00;    //normal mode
	TCCR1B_R = 0x0C;    //prescaler = 256 and set on compare match
	TCNT1_R  = 0;       // counter value to zero
	OCR1A_R  = 62500-1; // output compare value
	TIMSK1_R = 0x02;    //enabling output compare A interrupt
	
	sei(); //enable global interrupt
	
}

ISR (TIMER1_COMPA_vect){
		get_temp();
		print_values();
}

void spi_init(){
	DDRB_R = 0x07; //Pin0(SS) and Pin2(MOSI) are output
	SPC_R  = 0x51; //SPI enable,CPOL and CPHA are zero, master
	PORTB_R = 1;//pin0(SS) is high
}

void uart_init(){
	UCSR0_B = 0x18; //rx and tx enable
	UCSR0_C = 0x06; //8-bit data
	UBBR_0  = 104;  //Baud rate 9600
}

void uart_tx(uint8_t data){
	while(!(UCSR0_A & (1<<5)));
	UDR_0 = data;
}

uint8_t spi_trans(uint8_t data){
	SPD_R = data;
	while(!(SPS_R & (1<<7)));
	return SPD_R;
}

void get_temp(){
	PORTB_R &= ~(0x01);
	spi_trans(0xFA|0x80);
	for(int i=0;i<3;i++){
		data_buffer[i] = spi_trans(0x00);
	}
	PORTB_R |= 0x01;
}

long temp_calc(long adc_T){
	
	long int t_fine;
	
	long signed int var1, var2, T;
	unsigned short dig_T1 = 27504;
	
	short dig_T2 = 26435;
	
	short dig_T3 = -1000;
	
	var1 = ((((adc_T >> 3) - ((long int)dig_T1 << 1))) * ((long int)dig_T2)) >> 11;
	
	var2 = (((((adc_T >> 4) - ((long int)dig_T1)) * ((adc_T >> 4) - ((long int)dig_T1))) >> 12) * ((long int)dig_T3)) >> 14;
	
	t_fine = var1 + var2;
	
	T = (t_fine * 5 + 128) >> 8;
	
	return T;
}

void print_values(){
	long raw_temp = (((long)data_buffer[0]<<12) | ((long)data_buffer[1]<<4)|((long)data_buffer[2]>>4))& 0xFFFFFFFF ;
	int  temp_val = temp_calc(raw_temp);
	
	char str1[20];
	
	sprintf(str1,"%d",temp_val);
	
	for(int i=0;i<4;i++){
		if(i==2){
			uart_tx('.');
		}
		uart_tx(str1[i]);
	}
	uart_tx('C');
	uart_tx('\n');
	
}


