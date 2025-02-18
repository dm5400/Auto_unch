/*
 * arduino.c
 *
 * Created: 10.01.2025 21:09:36
 *  Author: Professional
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>
#include <stdlib.h>
#include <stdbool.h>

#define SPI_MISO 4 //PB4

#define SPI_MOSI 3 //PB3
#define SPI_SCK 5 //PB5
#define SPI_SS 2 //PB2
#define ADC_reg 0 //PC0

volatile unsigned char dac_type;

unsigned int read_adc(void)
{
	unsigned int a,b;
	ADCSRA |= (1<<ADSC); // начать оцифровку
	while (ADCSRA & (1<<ADSC)); // ждать оцифровки
	a=(ADCL>>6); 
	b=(ADCH<<2);
	a=a|b; // формирование слова  АЦП
	
	return a;
}

uint8_t SPI_ReadByte(uint8_t data)
{
	uint8_t report;
	 if (dac_type==0) PORTB &= ~(1<<SPI_SS);
     else PORTB |= (1<<SPI_SS);
   // PORTB &= ~(1<<SPI_SS);
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
	report = SPDR;
	//PORTB |= (1<<SPI_SS); не стирать
	_delay_us(1);
	return report;
}
void Timer0_init(void) {
    TCCR0A = 0;              // Нормальный режим работы таймера
    TCCR0B |= (1 << CS02) | (1 << CS00); // Предделитель 1024
    TIMSK0 |= (1 << TOIE0);  // Разрешение прерывания по переполнению
}

ISR(TIMER0_OVF_vect) {
    static uint16_t counter = 0;
     volatile unsigned int sum,tmp;
	volatile unsigned int res;
    char q;
    counter++;
    if (counter >= 18) {        // 12 переполнений ~ 300 мс при предделителе 1024
        counter = 0;
       q=0;
		sum=0;
      while (q<64) // 64 выборки от ацп
		{
			res=read_adc();
			res&= ~(1<<9);  //Tt=1/(0.003354+0.0006084*log(Rt/6.7826)+0.00001434*abs(pow(log(Rt/6.7826),2.3)))-273;
			sum+=res;
			q++;
		}  
    sum=sum/64;
    if (dac_type==0) // если оборев МПП закончен
						{//SPCR &= ~(1<<CPOL);
                       // SPCR &= ~(1<<CPHA);
                       // SPCR |= (1<<DORD);
                        sum=(sum<<2)&0x3FFF;
						SPI_ReadByte(sum);
                        PORTB |= (1<<SPI_SS);
                        sum=sum|0xC000;
                        SPI_ReadByte(sum);
                        PORTB |= (1<<SPI_SS);
						}
    else
    {   //SPCR |= (1<<CPOL);
        //SPCR |= (1<<CPHA);
        //SPCR &= ~(1<<DORD);
        tmp=(sum>>8);
        SPI_ReadByte(tmp);
        PORTB &= ~(1<<SPI_SS);
        SPI_ReadByte(sum);
        PORTB &= ~(1<<SPI_SS);
    }
    }
}

ISR(INT0_vect)
{ if (PIND & (1 << PIND2))
    {                 SPCR &= ~(1<<CPOL);    
                      SPCR &= ~(1<<CPHA);
                      SPCR |= (1<<DORD);
                      dac_type=0;
                       PORTD |= (1<<6);
                        
    }
    else
    { SPCR |= (1<<CPOL);
        SPCR |= (1<<CPHA);
        SPCR &= ~(1<<DORD);
        dac_type=1;
        PORTD &= ~(1<<6);
    }
}

ISR(INT1_vect)
{ if (PIND & (1 << PIND3))
    { 
    TIMSK0 &= ~(1 << TOIE0);             
                        
    }
    else
    { 
    TIMSK0 |= (1 << TOIE0);
    }
}
int main(void)
{  
    DDRB  |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS); // ???????????? ???
    DDRC = 0x00;
    DDRD = 0xe0;
    PORTD = 0xe0;
    SPCR |= (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(1<<SPR1)|(1<<SPR0);// spr0-1 spr1-0 fck/8
    SPSR = (1<<SPI2X);
    PORTD |= (1<<2);// INT0 PD2 подтяжка
    PORTD |= (1<<3);// INT1 PD3 подтяжка
    PORTB |= (1<<1); //удалить
    ADCSRA = 0x00; //disable adc
	ADMUX = 0b01110000; //00-выбор AREF, 1- Выравнивание по левому краю, 10000- ADC0-pos, ADC1-neg.
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); // Aden- adc on, предделитель равен 128
	//ADCSRA |= (1<<ADSC);//запуск одиночного преобразования
    EICRA &= ~(1 << ISC01); //Сбрасываем ISC01
    EICRA &= ~(1 << ISC11); //Сбрасываем ISC11
    EICRA |= (1 << ISC10);
  EICRA |= (1 << ISC00); // Устанавливаем ISC00 
  EIMSK |= (1 << INT0); // Разрешаем прерывание INT0
  EIMSK |= (1 << INT1); // Разрешаем прерывание INT0
  Timer0_init();
  sei();
    while(1)
    {   
    }
}