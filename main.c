#warning "ATMega16 Program Memory = 16kB  RAM= 1kB  EEPROM=512"
///////////////////////////////////////////////////////////////
#include <avr\io.h>              // Most basic include files
#include <avr\interrupt.h>       // Add the necessary ones
#include <stdlib.h>
#include <avr\pgmspace.h>


#include "clock.h"
#include "resources.h"
#include "one_wire.h"


void KeyboardRescan (void);
//////////////////////////////////////////////////////////////////////////////
//clock.c
extern STime date;
extern void ClockReset(void);
extern void TimeINC(void);
extern void ChangeDATE (unsigned char change_mode,unsigned char index);
 
//////////////////////////////////////////////////////////////////////////////
//one_wire.c
extern unsigned char 	OneWire_TestPresence(void);
extern void 			OneWire_Send_Byte(unsigned char Data);
extern unsigned char 	OneWire_Recieve_Byte(void);

//--
extern SRecord srRecordBuffer; //буфер хранения записи для редактирования и вывода на экран
//--
extern unsigned char indexCurrentRecord; //индекс текущей отображаемой записи (которая хранится в RecordBuffer)
extern unsigned char RecordsNumber;//количество не стертых записей в ееprоm
extern unsigned char indexLastDeleted;//индекс первой удаленной записи в списке
//--
extern unsigned char indexMinimorum;
extern unsigned char indexStored_L;
extern unsigned char indexStored_R;
extern unsigned char indexMaximorum;
extern unsigned char record_position;
//--
extern void SaveRecord (unsigned char index);
extern void LoadRecord (unsigned char index);
extern void eepromReadBuffer(unsigned int address, unsigned char *buffer, unsigned char length);
extern void eepromWriteBuffer(unsigned int address, unsigned char *buffer, unsigned char length);
extern void eepromClear(void);
extern void ChangeERecord(unsigned char change_mode,unsigned char index);
extern void AddRecord(void);
extern unsigned char DeleteCurrentRecord(void);
extern unsigned char EventPolling(void);
extern unsigned char IndexesUpdate (unsigned char indexSource);
//
//////////////////////////////////////////////////////////////////////////////

extern unsigned char 	eventTemperature;
extern unsigned char	eventAttribures;


unsigned char intState;

unsigned char CurrentPanel=1;//текущая активная панель
unsigned char DefaultPanel=0;
unsigned char CursorBindPanel=0;
unsigned char NewPanel=0;//которая будет установлена

unsigned char time_dcnt=0;
unsigned char time_state=0;

unsigned char tick_dcnt=0;
unsigned char tick_state=0;
unsigned char tick_value=0;

unsigned char alarm_dcnt=0;
unsigned char alarm_state=0;

unsigned char kbd_dcnt =0;
unsigned char kbd_state=0;

unsigned char que1wire_dcnt=0;
unsigned char que1wire_state=0;

unsigned char quePWR_dcnt=0;
unsigned char quePWR_state=0;

unsigned char polling_dcnt =0;
unsigned char polling_state=0;

unsigned char relay_dcnt=0;
unsigned char relay_state=0;

unsigned char led_dcnt=0;
unsigned char led_state=0;

unsigned char panel_dcnt=0;
unsigned char panel_state=0;
unsigned char panel_period=1;
unsigned char kbd_period=0;	

unsigned char relay_mode_manual =0;
unsigned char relay_mode_auto=0;


unsigned char powersafe_enable=0;
unsigned char Termostat (void); //функция термостата
unsigned char termostat_setting; //температура уставки термостата
unsigned char pwr_voltage=0xFF;
unsigned char usrEvent=0;


void MapPanels(void);
unsigned char PanelsMap[PANELS]={0}; //смещение для панелей в файле панелей

////////////////////////////////////////////////////////////////
unsigned char GBuffer[320]={0}; 
unsigned char ScratchPad[9]={0};
////////////////////////////////////////////////////////////////
unsigned char TLogStorage[80]={0};//буфер лога температуры
unsigned char CurrentTLogIndex; //текущая позиция лога температуры
////////////////////////////////////////////////////////////////

unsigned char CursorsMap[4*13]={0};
unsigned char nCursors=0;
unsigned char cursors_update=0;//включение режима обнаружения курсоров
unsigned char skip_cursor=0; //включение режима пропуска курсора
unsigned char CurrentCursor=1;
////////////////////////////////////////////////////////////////
void Beep (void);
void RenderPanel(void);
void RenderChar(unsigned char X, unsigned char Y, unsigned char DrawMode, unsigned char CharCode);
void Pixel (unsigned char X, unsigned char Y);
unsigned char SPI_Transfer(unsigned char OutData);
/////////////////////////////////////////////////////////////////
unsigned char panel_dirty;
unsigned char bufferSPI_complete=1;
/////////////////////////////////////////////////////////////////////////////
//вспомогательные процедуры
static  void _DELAY_NCLK(short __count) __attribute__((always_inline));
static  void _DELAY_NCLK(short __count)
{//холстые такты задержки
__asm__ __volatile__ 
(
		"L_loop%=: 		sbiw 	r24,1 				\n\t" //2clk (0.25мкс при F_CPU=8MHz
		"				brne 	L_loop%=			\n\t" //2clk  	
://output operand list
://input operand list
//://[clobber list]						
);														  
return;
}

static  void _SLEEP(void) __attribute__((always_inline));
static  void _SLEEP(void) 
{
	MCUCR |= (1<<SE);
	asm volatile("sleep");
}

////////////////////////////////////////////////////////////////
EMPTY_INTERRUPT(INT0_vect);			// External Interrupt Request 0 		_VECTOR(1)
EMPTY_INTERRUPT(INT1_vect);			// External Interrupt Request 1 		_VECTOR(2)

//EMPTY_INTERRUPT(TIMER2_COMP_vect);	// Timer/Counter2 Compare Match 		_VECTOR(3)
unsigned char TC2_LoopCounter;

ISR(TIMER2_COMP_vect)	// Timer/Counter2 Compare Match		_VECTOR(3)
{//call function clock update

		///////////////////////////////////////////////////////////
		//счет часов реального времени
		//--  1 сек 16	
		if (time_dcnt ==0)
		{//событие 1 секунда 
			TimeINC();		//приращение времени каждую секунду  
			time_dcnt =16; //каждый 16й вызов - 1  сек 16call/sec=(zq=32768)/(presc=128)/(ocr2=16) 
			time_state++;					
		};
		time_dcnt--;
		//--
		if (tick_dcnt ==0)
		{//событие 0.5 секунды
			tick_dcnt=8;
			tick_state++;	
		};
		tick_dcnt--;
		/////////////////////////////////////////////////////////
		//обслуживание счетчика задержки
		TC2_LoopCounter--;

		///////////////////////////////////////////////////////////
		//генерация прочих событий

//--
		if (quePWR_dcnt==0)
		{
			quePWR_dcnt =16;
			quePWR_state++;	
		};
		quePWR_dcnt--;
//-- 
		if (que1wire_dcnt==0)
		{
			que1wire_dcnt=16;  
			que1wire_state++;	
		};
		que1wire_dcnt--;
//--
		if (kbd_dcnt==0)
		{
			kbd_dcnt=1;
			kbd_state++;
		};		
		kbd_dcnt--;
//--
		if (polling_dcnt==0)
		{
			polling_dcnt =16;
			polling_state++;
		};
		polling_dcnt--;

//--
		if (relay_dcnt==0)
		{
			relay_dcnt = 1;
			relay_state++;
		};
		relay_dcnt--;
//--		
		if (led_dcnt==0)
		{//разрешить обновление состояния светодиода
			led_dcnt =1;
			led_state++;	
		};
		led_dcnt--;

//--   
		if (panel_dcnt==0)
		{
			panel_dcnt=1;
			panel_state++;
		};
		panel_dcnt--;
//--
	
		if (time_dcnt==15)
		{
			alarm_dcnt=8;
		};		
		if (alarm_dcnt ==0)
		{	
			alarm_dcnt=2;
			if (alarm_state){Beep();}; 
		};
		alarm_dcnt--;

//--		

		
return;
}



EMPTY_INTERRUPT(TIMER2_OVF_vect);	// Timer/Counter2 Overflow 			_VECTOR(4)


EMPTY_INTERRUPT(TIMER1_CAPT_vect);	// Timer/Counter1 Capture Event 		_VECTOR(5)

//EMPTY_INTERRUPT(TIMER1_COMPA_vect);	// Timer/Counter1 Compare Match A 	_VECTOR(6)
	unsigned int CurrentOCRA=0;
	ISR(TIMER1_COMPA_vect)					// Timer/Counter1 Compare Match A 	_VECTOR(6)
	{
		//частота
		CurrentOCRA +=0x002F;//масштаб частоты
		OCR1A=CurrentOCRA;
		return;
	}


//EMPTY_INTERRUPT(TIMER1_COMPB_vect);	// Timer/Counter1 Compare Match B 	_VECTOR(7)
	unsigned int CurrentOCRB=0;
	unsigned int DurationCounter=0;
	ISR(TIMER1_COMPB_vect)					// Timer/Counter1 Compare Match B 	_VECTOR(7)
	{
		//длительность
		CurrentOCRB += 0x002F;//масштаб длительности
		OCR1B=CurrentOCRB;
		DurationCounter--;
		if(DurationCounter==0)
		{
			DDRD  &= ~(1<<PD5);		//set OC1A as input
			TIMSK &= ~(1<<OCIE1A); //Output Compare A Match Interrupt Disable
			TIMSK &= ~(1<<OCIE1B); //Output Compare B Match Interrupt Disable
		}
		return;
	}

EMPTY_INTERRUPT(TIMER1_OVF_vect);	// Timer/Counter1 Overflow			_VECTOR(8)


//EMPTY_INTERRUPT(TIMER0_OVF_vect);	// Timer/Counter0 Overflow 			_VECTOR(9)
unsigned char chunk_num =0;
unsigned char page_num =0;
unsigned int aindex=0;

ISR(TIMER0_OVF_vect)				// Timer/Counter0 Overflow 			_VECTOR(9)
{
/////////////////////////////////////////////////////////
//прочие системные задачи
/////////////////////////////////////////////////////////
//обновление экрана
unsigned char n;

//GBuffer2[4][80]
if (aindex ==0) {bufferSPI_complete =0;};		//начата выгрузка буфера в индикатор
	PORTB &=~(1<<PB3);							//BO8032 режим команд ~LCD_R/S = 0
	if (chunk_num==0)
	{
		chunk_num=8*2;
		SPI_Transfer(0x40);						//[Initial Display Line] 0x40...0x40+0x3F
		SPI_Transfer(0x13);						//[Set Column Address]	0-131	//msb Y7 Y6 Y5 Y4 
		SPI_Transfer(0x04);						//[Set Column Address]	0-131	//lsb Y3 Y2 Y1 Y0
		SPI_Transfer(0xB0+page_num);			//Set Page Address 0xB0...0xB0+0x0F
		page_num++;	
			
		if (page_num > 0x03)
		{	
			page_num =0x00;
		};
	};
	PORTB |= (1<<PB3);								//BO8032 режим данных ~LCD_R/S = 1 
	PORTB &=~(1<<PB4);								//SelectSlave set ~LCD_CS = 0	
	for (n=0;n<5;n++)
		{
			SPSR  &= ~(1<<SPIF);				//
		    SPDR = GBuffer[aindex];				//
			while (!(SPSR & (1<<SPIF)));		// wait for transfer is complete
			aindex++;
		};
	PORTB |= (1<<PB4);							//ReleaseSlave set ~LCD_CS = 1
	
	chunk_num--;
	if (aindex>319){
	bufferSPI_complete =1;						//выгрузка буфера в индикатор завершена
	aindex = 0;
	TIMSK &=~(1<<TOIE0); 		//Overflow Interrupt Disable		
	};

//////////////////////////////////////////////////////////
//
	return;
}

EMPTY_INTERRUPT(SPI_STC_vect);		// Serial Transfer Complete 		_VECTOR(10)
EMPTY_INTERRUPT(USART_RXC_vect);	// USART, Rx Complete 				_VECTOR(11)
EMPTY_INTERRUPT(USART_UDRE_vect);	// USART Data Register Empty 		_VECTOR(12)
EMPTY_INTERRUPT(USART_TXC_vect);	// USART, Tx Complete 				_VECTOR(13)

//EMPTY_INTERRUPT(ADC_vect);			// ADC Conversion Complete 			_VECTOR(14)
ISR(ADC_vect)			// ADC Conversion Complete 			_VECTOR(14)
{
	pwr_voltage = ADCL;			//прочитать значение напряжения источника
	pwr_voltage = ADCH;			//прочитать значение напряжения источника
	ADCSRA &= ~(1<<ADIE); 		//запретить это прерывание
	ADCSRA &=~(1<<ADEN);		    //включить АЦП(1- АЦП вкл.)
	PORTA &=~(1<<PA1);		//set ADC_V_BAT_EN =0 -> отключть источник от входа АЦП	


return;
}


EMPTY_INTERRUPT(EE_RDY_vect);		// EEPROM Ready 					_VECTOR(15)
EMPTY_INTERRUPT(ANA_COMP_vect);		// Analog Comparator 				_VECTOR(16)
EMPTY_INTERRUPT(TWI_vect);			// 2-wire Serial Interface 			_VECTOR(17)
EMPTY_INTERRUPT(SPM_RDY_vect);		// Store Program Memory Ready 		_VECTOR(18)
/////////////////////////////////////////////////////////////////////////////////////////////
//


void TC2_Loop(unsigned char IntCount);
void TC2_Loop(unsigned char IntCount)
{//задержка по количеству прерываний по TC2 
	TC2_LoopCounter = IntCount;
	while (TC2_LoopCounter);
}

#define  KBD_TIMEOUT	0xA0

unsigned char kbd_cmd;

void KeyboardRescan (void)
{
static unsigned char kbd_timeout;
static unsigned char kbd_last_value;
static unsigned char kbd_new_value;
		SFIOR &=~(1<<PUD); // ==0 //разрешить подтяжку глобально	
			DDRA &= ~((1<<PA2)|(1<<PA4)|(1<<PA5)|(1<<PA6)|(1<<PA7));//kbd inputs
			DDRA |= (1<<PA3);//kbd outputs
			PORTA |= (1<<PA2)|(1<<PA4)|(1<<PA5)|(1<<PA6)|(1<<PA7);//разрешить подтяжку по входам
			PORTA &=~(1<<PA3);
			asm volatile("nop"::);
			kbd_new_value = ~PINA; 
			kbd_new_value &= 0xF4; //фильтрация битов		
		SFIOR |=(1<<PUD); // ==1 //запретить подтяжку глобально

		kbd_timeout++;
		if(kbd_timeout>KBD_TIMEOUT) {kbd_timeout=0;kbd_cmd=6;};//событие 6 таймаут клавиатуры
		
		if ((kbd_new_value != kbd_last_value)&&(kbd_new_value!=0))
		{
					switch (kbd_new_value)
					{
						case 0x80:{kbd_cmd=1;}break;//up
						case 0x04:{kbd_cmd=2;}break;//right
						case 0x20:{kbd_cmd=3;}break;//left
						case 0x40:{kbd_cmd=4;}break;//down
						case 0x10:{kbd_cmd=5;}break;//ok
						default:  {kbd_cmd =0;}break;//комбинация ничего не значит
					};
					kbd_timeout=0; //состояние обновлено, сброс таймаута
					Beep();
		};						
		kbd_last_value = kbd_new_value;	
return;
}


///////////////////////////////////////////////////////////////////////////////////////
//void Beep (void);//__attribute__ ((naked));
void Beep (void)
{
		if (DurationCounter==0)
		{
			DurationCounter =0x002F;  //количество тиков
			CurrentOCRA = 0x002F;//масштаб частоты
			CurrentOCRB = 0x002F;//масштаб длительности
			intState = 0x80 & SREG;
			cli ();	
				OCR1A=CurrentOCRA;
				OCR1B=CurrentOCRB;
				TCNT1=0x0000;
				TIMSK |= (1<<OCIE1A); //Output Compare A Match Interrupt Enable
				TIMSK |= (1<<OCIE1B); //Output Compare B Match Interrupt Enable
				DDRD  |=   (1<<PD5);		//set OC1A as output
			SREG |= intState;
		}
	return;
}



///////////////////////////////////////////////////////////////////////////////////////
//
//char SPI_Transfer(char OutData);
unsigned char SPI_Transfer(unsigned char OutData)
 {		
		PORTB &=~(1<<PB4);					//SelectSlave set ~LCD_CS = 0
			SPSR  &= ~(1<<SPIF);				// transfer begin			
			SPDR = OutData;						
			while (!(SPSR & (1<<SPIF)));		// wait for transfer is complete
		PORTB |= (1<<PB4);					//ReleaseSlave set ~LCD_CS = 1
		return SPDR;					// return the received data
 }


   
void SwitchLED(unsigned char led_value);
void SwitchLED(unsigned char led_value)
{//управление двухцветным светодиодом
	switch (led_value & 0x03)
	{
		case 0:{//все выключено  00
			PORTC &=~((1<<PC2)|(1<<PC3));
		}break;
		case 1:{//левый цвет 	 01
			PORTC |= (1<<PC2);PORTC &=~(1<<PC3);		
		}break;
		case 2:{//правый цвет    10
			PORTC &=~(1<<PC2);PORTC |= (1<<PC3);
		}break;
		case 3:{//все включено   11
			PORTC |= ((1<<PC2)|(1<<PC2));
		}break;
		default:break;
	}
	return;
}

/////////////////////////////////////////////////////////////
#define 	RELAY_OFF			0x00    
#define 	RELAY_ON			0x01			
#define 	RELAY_INVERT		0x02    
#define 	RELAY_KEEP			0x03    

unsigned char relay_last_state;

void SwitchRelay (unsigned char relay_new_state);
void SwitchRelay (unsigned char relay_new_state)
{//бистабильное реле с одной обмоткой
	//--
		switch (relay_new_state)
		{
			case RELAY_INVERT:{relay_new_state = (~relay_last_state) & RELAY_ON;}break;
			case RELAY_KEEP:{relay_new_state = relay_last_state;}break;
			default:{}break;
		};
	//--
		switch (relay_new_state)
		{
			case RELAY_OFF:{PORTB |= (1<<PB0);PORTB &=~(1<<PB1);}break;
			case RELAY_ON:{PORTB |= (1<<PB1);PORTB &=~(1<<PB0);}break;
			default:{}break;
		};
	//--	
		_DELAY_NCLK(0x0FFF);	//на время реакции реле
		PORTB &=~(1<<PB0);		// 
		PORTB &=~(1<<PB1);		//
		relay_last_state = relay_new_state;	
	return;
}
//////////////////////////////////////////////////////////////


unsigned char BCD[13];

void Int2BCD2 (unsigned int N,unsigned char len);
void Int2BCD2 (unsigned int N,unsigned char len) 
{
	unsigned char n;
	unsigned char *b;
	b=&BCD[0];
	for(n=len; n>0; n--){*(b+n-1) = N%10+0x30; N = N/10;};//224 байта
	return;
}



int main(void)
{

//goto debug_start;

cli ();
//////////////////////////////////////////////////////////////////////
//установка режима sleep
MCUCR &=~((1<<SM2)|(1<<SE)|(1<<SM1)|(1<<SM0));
MCUCR |=0//				//PWR Save
//		|(1<<SM2)		//0
		|(1<<SM1)		//1
		|(1<<SM0)		//1
;

//MCUCR &=~(1<<SE);//игнорировать команду SLEEP
//MCUCR |= (1<<SE);//команда SLEEP активна

//начальное состояние системы управления eeprom
EECR = 0;

//////////////////////////////////////////////////////////////////////
//настройка таймера TC0: 			
		SFIOR = SFIOR
				|(1<<PSR10)		//Prescaler Reset 1=reset
;									//		*
		TCCR0=0	//Clock Select    	//OFF	clk/1	clk/8	clk/64	clk/256	clk/1024	|T0-rize	T0-fall //
			|(1<<CS00)				//0		1		0		1		0		1			|	0		1		//
//			|(1<<CS01)				//0		0		1		1		0		0			|	1		1		//
//			|(1<<CS02)				//0		0		0		0		1		1			|	1		1		//
;
	 	TIMSK = TIMSK
//	 			|(1<<TOIE0)		//Overflow Interrupt Enable
;    	 
	 	SFIOR = SFIOR
				|(1<<PSR10)		//Prescaler Reset 1=reset
;
//////////////////////////////////////////////////////////////////////
//настройка выводов контроллера для работы с TC0
//    	DDRB  &=  ~(1<<PB0);		//set PB0(XCK/T0) as input (when use external clock)				 
//    	DDRB  &=  ~(1<<PB3);		//set PB3(AIN1/OC0) as Output (when use Compare Match Output)
//////////////////////////////////////////////////////////////////////
//настройка таймера TC1 
//(генератор звука) 
	SFIOR = SFIOR		//Special Function IO Register
//			|(1<<PSR10)	//Prescaler Reset Timer/Counter1 and Timer/Counter0
;
	TIFR  = TIFR//Timer/Counter Interrupt Flag Register – 
//			|(1<<ICF1)		//Input Capture Flag 
//			|(1<<OCF1A)		//Output Compare A Match Flag 
//			|(1<<OCF1B)		//Output Compare B Match Flag 
//			|(1<<TOV1)		//Overflow Flag 
;
	TIMSK = TIMSK //Timer/Counter Interrupt Mask Register
//			|(1<<TICIE1) 	//Input Capture Interrupt Enable
//			|(1<<OCIE1A) 	//Output Compare A Match Interrupt Enable
//			|(1<<OCIE1B)  	//Output Compare B Match Interrupt Enable
//			|(1<<TOIE1)  	//Overflow Interrupt Enable
;
	TCCR1A= 0//Timer/Counter 1 Control Register A

//			|(1<<COM1A1) 	//7//Compare Output Mode for channel A 1   
			|(1<<COM1A0) 	//6//Compare Output Mode for channel A 0  //Toggle on compare
//			|(1<<COM1B1) 	//5//Compare Output Mode for channel B 1
//			|(1<<COM1B0) 	//4//Compare Output Mode for channel B 0
//			|(1<<FOC1A)  	//3//Force Output Compare for channel A
//			|(1<<FOC1B)  	//2//Force Output Compare for channel B
//			|(1<<WGM11)  	//1//Waveform Generation Mode 1
//			|(1<<WGM10)  	//0//Waveform Generation Mode 0
;
		TCCR1B=0//Timer/Counter 1 Control Register B
//			|(1<<ICNC1)		//Input Capture Noise Canceler
//			|(1<<ICES1)		//Input Capture Edge Select 
//			|(1<<WGM13)		//Waveform Generation Mode 3
//			|(1<<WGM12)		//Waveform Generation Mode 2
												//							*
												// OFF	clk/1	clk/8	clk/64	clk/256 //
//			|(1<<CS12)		//Clock Select 2	//	0		0		0		0		1 	//		
			|(1<<CS11)		//Clock Select 1	//	0		0		1		1		0	//
			|(1<<CS10)		//Clock Select 0	//	0		1		0		1		0	//
;
//////////////////////////////////////////////////////////////////////
//настройка выводов контроллера для работы с TC1
//    	DDRD  &=  ~(1<<PB1);		//set PB1(T1) as input (when use external clock)	
//    	DDRD  &=  ~(1<<PD6);		//set PB6(ICP1) as input 	
//    	DDRB  |=   (1<<PD5);		//set OC1A as output 	
//    	DDRB  |=   (1<<PD4);		//set OC1B as output 	

//////////////////////////////////////////////////////////////////////
//настройка таймера TC2 
//: асинхронный режим, внешний кварц
//:
		OCR2 = 0x0F; // при T=32768/128=256имп/сек   OCR2=16 -  прерывание по 16му импульсу  (256/16 - 16прерыв/сек)
					 // {ZQ/(N[CS20:CS22])}/OCR2    

		TCCR2=0					//Timer/Counter Control Register
//			  |_BV(FOC2)	//7//Force Output Compare
//			  |_BV(WGM20)	//6//Waveform Generation Mode
//			  |_BV(COM21)	//5//Compare Match Output Mode
//			  |_BV(COM20)	//4//Compare Match Output Mode
			  |_BV(WGM21)	//3//Waveform Generation Mode
														//										*
														// OFF	c/01	c/08	c/32  c/64		c/128	c/256	c/1024	//
			  |_BV(CS22)	//2//R/W//0//Clock Select	//	0	0		0		0		1		1		1		1		//
//			  |_BV(CS21)	//1//R/W//0//Clock Select	//	0	0		1		1		0		0		1		1		//
			  |_BV(CS20)	//0//R/W//0//Clock Select	//	0	1		0		1		0		1		0		1		//
;
		ASSR =0					//Asynchronous Status Register
			  |_BV(AS2)		//3//R/W//0//Asynchronous Timer/Counter2 1-TOSC 2-OTHER	
		//	  |_BV(TCN2UB)	//2//R  //0//TCNT2 Update Busy
		//	  |_BV(OCR2UB)	//1//R  //0// OCR2 Update Busy	
		//	  |_BV(TCR2UB)	//0//R  //0//TCCR2 Update Busy
;
		TIMSK=TIMSK				//Timer/Counter Interrupt Mask Register
			  |_BV(OCIE2)	//7//R/W//0//Timer/Counter2 Output Compare Match Interrupt Enable
//			  |_BV(TOIE2)	//6//R/W//0//Timer/Counter2 Overflow Interrupt Enable
;
		TIFR =TIFR				//Timer/Counter Interrupt Flag Register
			  |_BV(OCF2)	//7//R/W//0//Output Compare Flag 2
//			  |_BV(TOV2)	//6//R/W//0//Timer/Counter2 Overflow Flag
;
//////////////////////////////////////////////////////////////////////
//настройка выводов контроллера для работы с TC2	
//    	DDRD  &=  ~(1<<PD7);		//set OC2 as output (when use Compare Match Output)	

//////////////////////////////////////////////////////////////////////
//настройка SPI: 
		SPCR =	0
//				|	(1<<SPIE)	//7//SPI Interrupt Enable  (1- enable 0 - disable)
				|	(1<<SPE)	//6//SPI Enable (1-enable 0-disable)
//				|	(1<<DORD)	//5//Data Order (1 -LSB first 0-MSB-first)
				|	(1<<MSTR)	//4//Master/Slave Select (1-master 0-slave)
//				|	(1<<CPOL)	//6//Clock Polarity (1-negative polarity 0-positive polarity)
//				|	(1<<CPHA)	//2//Clock Phase (0-fore front 1-back front)
//				|	(1<<SPR1)	//1//SPI Clock Rate Select 1			 0 0 1 1  'f/4'f/16'f/64'f/128'  SPI2X =0
//				|	(1<<SPR0)	//0//SPI Clock Rate Select 0			 0 1 0 1  'f/2'f/8 'f/32'f/64 '  SPI2X =1 double speed
;
		SPSR =	0
//				|	(1<<SPIF)	//7//SPI Interrupt Flag    (1- transfer complete or MCU pass to Slave from Master mode )
//				|	(1<<WCOL)	//6//Write COLlision Flag  (1- was collision write SPDR when SPIF = 0)
				|	(1<<SPI2X)	//0//Double SPI Speed Bit  (0-single speed 1-double speed)
;
//////////////////////////////////////////////////////////////////////
//настройка выводов контроллера для работы с SPI
		DDRB  |= (1<<PB7);		// set SCK as output(LCD_SCLK)
		DDRB  &=~(1<<PB6);		// set MISO as input(PR_MISO)
		DDRB  |= (1<<PB5);		// set MOSI as output(LCD_SID)
		DDRB  |= (1<<PB4);		// set SS as output in MasterMode



//////////////////////////////////////////////////////////////////////
//настройка ADC: 

		  ADMUX &=~((1<<REFS1)|(1<<REFS0));	
		  ADMUX |=0//источник опорного напряжения       *
//		  			|(1<<REFS1)					//		0	0	1	1
//					|(1<<REFS0)					//		0	1	0	1
					;
		  ADMUX &=~(1<<ADLAR);				
		  ADMUX |=0 //результат выровнять по старшему биту (Left Adjusted Result)
		  			|(1<<ADLAR)				
					;
		  ADMUX &=~((1<<MUX4)|(1<<MUX3)|(1<<MUX2)|(1<<MUX1)|(1<<MUX0));	
		  ADMUX |=0//выбор канала 0x00...0x07 (0x00...0x1F) 00011111 
//		  			|(1<<MUX4)
//					|(1<<MUX3)
//					|(1<<MUX2)
//					|(1<<MUX1)
//					|(1<<MUX0)
					;		  
	  	  SFIOR &=~((1<<ADTS2)|(1<<ADTS1)|(1<<ADTS0));
	  	  SFIOR |=0 //источник автозапуска преобразования
		  			|(1<<ADTS2)   //запуск по переполнению T0 
//					|(1<<ADTS1)
//					|(1<<ADTS0)
					;		  
		  ADCSRA &= ~(1<<ADATE);
		  ADCSRA |= 0 //автозапуск преобразования
		  			|(1<<ADATE)				
					;
		  ADCSRA &= ~(1<<ADIE);
		  ADCSRA |= 0 //вызов прерывания ISR(ADC_vect){}; по окончанию преобразования
//		  			|(1<<ADIE)				
					;
		  ADCSRA &= ~((1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0));
		  ADCSRA |= 0 //частота преобразователя 	16
		  			|(1<<ADPS2)					//	1
//					|(1<<ADPS1)					//	0
//					|(1<<ADPS0)					//	0
					;
	//	 ADCSRA |=(1<<ADEN);		    //включить АЦП(1- АЦП вкл.)		
	//			  ADCSRA |=(1<<ADSC); 			//запустить преобразователь АЦП      
	//			  while((ADCSRA & ADIF) ==0){}; //ожидать завершения преобразования
	//			  pwr_voltage = ADCL;		   	//прочитать значение напряжения источника
	//			  pwr_voltage = ADCH;		   	//прочитать значение напряжения источника
	//	 ADCSRA &=~(1<<ADEN);			//отключить АЦП(0- АЦП выкл.)	

//////////////////////////////////////////////////////////////////////
//настройка USART: 

//#define FOSC 1843200// Clock Speed
//#define BAUD 9600
//#define MYUBRR FOSC/16/BAUD-1

/* Set baud rate */
//UBRRH = (unsigned char)(MYUBRR>>8);
//UBRRL = (unsigned char)MYUBRR;
/* Enable receiver and transmitter */
//UCSRB = (1<<RXEN)|(1<<TXEN);
/* Set frame format: 8data, 2stop bit */
//UCSRC = (1<<USBS)|(1<<USBS)|(3<<UCSZ0);	
//XCK USART External Clock (Synchronous Transfer) 
//используется когда UMSEL = 1 автоматически
//	when Slave -> as Input    
//	when Master -> as Output 
//////////////////////////////////////////////////////////////////////
//настройка выводов контроллера для работы с USART
//RXD
//TXD
//XCK
//////////////////////////////////////////////////////////////////////
//настройка выводов для работы с RS232
//		PORTD &=~(1<<PA2);		//set RS_SHDN =0 -> состояние shutdown
//		DDRD  |=(1<<PD2);		//RS_SHDN	as output
//////////////////////////////////////////////////////////////////////
//Настройка Прочих Выводов
//////////////////////////////////////////////////////////////////////
//управление подтягивающими резисторами выводов
		SFIOR |=(1<<PUD); // ==1 //подтяжка запрещена глобально 
//////////////////////////////////////////////////////////////////////
//настройка выводов для управления светодиодами
		DDRC  |= (1<<PC2);		// set LED_RED as output
		DDRC  |= (1<<PC3);		// set LED_GREEN as output
//////////////////////////////////////////////////////////////////////
//настройка выводов для управления реле
		PORTB |= (1<<PB0);		// set RELAY = 1			
		PORTB |= (1<<PB1);		// set ~RELAY = 1			
		DDRB  |= (1<<PB0);		// set RELAY as output
		DDRB  |= (1<<PB1);		// set ~RELAY as output
//////////////////////////////////////////////////////////////////////
//настройка выводов для работы со схемой контроля батареи		
		PORTA &=~(1<<PA1);		//ADC_V_BAT_EN =0 -> состояние запрет измерения
		DDRA  |= (1<<PA1);		//as output
		PORTA &=~(1<<PA0);		//цифровой вход вывода АЦП в состояние Z
		DDRA  &=~(1<<PA0);		//as input		
//////////////////////////////////////////////////////////////////////
//настройка выводов для работы с клавиатурой		
//	см. void KeyboardRescan(void)
//////////////////////////////////////////////////////////////////////
//настройка выводов контроллера для управления BO8032
	    PORTB |= (1<<PB3);		// set LCD_R/S = 1			//режим приема BO8032 (прием данных)
	    DDRB  |= (1<<PB3);		// set LCD_R/S as output
		PORTB |= (1<<PB2);		// set LCD_RES = 1			//сигнал сброса BO8032 (не активен)
	    DDRB  |= (1<<PB2);		// set LCD_RES  as output
//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
//инициализация прериферийных устройств 
//////////////////////////////////////////////////////////////////////

//обеспечение сигнала  LCD_RES для BO8032
		PORTB &=~(1<<PB2);      
		_DELAY_NCLK(0xFFFF);
		PORTB |= (1<<PB2);

//инициализация индикатора BO8032
			PORTB &=~(1<<PB3);			//~LCD_R/S = 0	//BO8032 переход в режим команд		

			SPI_Transfer(0xE2);		//BO_RESET	
			SPI_Transfer(0xA0);		//BO_ADC_SELECT
			SPI_Transfer(0xC0);		//BO_SHL_SELECT 	
			SPI_Transfer(0xA2);		//BO_BIAS_SELECT_	//0xA2...0xA3  
			SPI_Transfer(0x2C);		//BO_VOLTAGE_CONVERTER_ON
			SPI_Transfer(0x2E);		//BO_VOLTAGE_REGULATOR_ON+BO_VOLTAGE_CONVERTER_ON
			SPI_Transfer(0x2F);		//BO_VOLTAGE_FOLLOWER_ON+BO_VOLTAGE_REGULATOR_ON+BO_VOLTAGE_CONVERTER_ON
			SPI_Transfer(0x23);		//BO_REGULATOR_RESISTOR_SELECT_//0x20...0x27
			SPI_Transfer(0x81);		//[Reference Voltage Register Set] 1
			SPI_Transfer(0x07);		//[Reference Voltage Register Set] 2
			SPI_Transfer(0xAF);		//BO_DISPLAY_ON	

///////////////////////////////////////////////////////////////////////
					TIMSK = TIMSK
				 	|(1<<TOIE0)		//Overflow Interrupt Enable
					;
		sei();
 debug_start:
		//УСТАНОВКА ЧАСОВ
		ClockReset();//сброс часов
		//ИНИЦИАЛИЗАЦИЯ ГРАДУСНИКА
		OneWire_TestPresence(); 				//Reset & Presense
		OneWire_Send_Byte(OW_SKIP_ROM);   		//передача команды "Skip ROM" (0xCC)														
		OneWire_Send_Byte(OW_CONVERT_T);    	//передача команды “Convert T” command

		Beep();  //сигнал предварительная готовность

unsigned char n;
		//////////////////////////////////////////////////////////////////////////////////////
		//сканирование файла панелей управления
		MapPanels();
		//////////////////////////////////////////////////////////////////////////////////////
		//подготовка списка событий к работе
		eepromReadBuffer(0, (unsigned char*)(&n), 1);
		if (n!=0xAA)//проверка первого байта еепром
		{//список отсутствует в EEPROM или поврежден -> инициализация нового справочника
			eepromClear(); 		//очистка EEPROM нулями
			n=0xAA;				//метка справочник присутствует
			eepromWriteBuffer(0, (unsigned char*)(&n), 1);
		};
		//список присутствует в EEPROM - анализ имеющегося справочника
		if (IndexesUpdate (0)==0xFF) 
		{//справочник пуст
			AddRecord();//автоматически добавить одну новую запись для шаблона
		}
		//IndexesUpdate (0);

//инициализация контроллера и периферийных устройств завершена
//////////////////////////////////////////////////////////////////////////////////////
		Beep();  //сигнал полная готовность

//MCUCR &=~(1<<SE);//игнорировать команду SLEEP
//MCUCR |= (1<<SE);//команда SLEEP активна
		//ADCSRA |=(1<<ADEN);		    //включить АЦП(1- АЦП вкл.)		  	
//запуск цикла приложения
do
	{
		
		
//	goto skip_application;	
		if (time_state>0) {time_state=0;};
		if (tick_state>0) {tick_value=~tick_value; tick_state=0;};
		
		if (quePWR_state>30+kbd_period)
		{
			quePWR_state=0;
			ADCSRA |=(1<<ADEN);		    //включить АЦП(1- АЦП вкл.)	
			PORTA |= (1<<PA1);		//ADC_V_BAT_EN =1 -> подключить источник ко входу АЦП
	  		ADCSRA |= (1<<ADIE);	  //разрешить прерывание
			continue;
		};

		if (que1wire_state>30+kbd_period)
		{		
				que1wire_state =0;								
				OneWire_TestPresence(); 				//Reset & Presense
				OneWire_Send_Byte(OW_SKIP_ROM);   		//передача команды "Skip ROM" (0xCC)	
				OneWire_Send_Byte(OW_READ_SCRATCHPAD);	//передача команды “Read Scratchpad” command.
				for (n=0;n<9;n++) 
				{//пинимаем 9x8bit из памяти DS1820.и складываем в массив ScratchPad[]
					ScratchPad[n] = OneWire_Recieve_Byte();
				};   
				OneWire_TestPresence(); 				//Reset & Presense
				OneWire_Send_Byte(OW_SKIP_ROM);   		//передача команды "Skip ROM" (0xCC)														
				OneWire_Send_Byte(OW_CONVERT_T);    	//передача команды “Convert T” command.
				CurrentTLogIndex++;
				if (CurrentTLogIndex>79) CurrentTLogIndex=0;
				//выборка температуры в диапазоне -60...0...+60 (старший бит ScratchPad[0] отброшен) 
				TLogStorage[CurrentTLogIndex] = (ScratchPad[1] & 0x80) | (ScratchPad[0] & ~0x80);
				continue; 		
		};

//--
unsigned char kbd_dirty=0;	

	if(kbd_state>kbd_period)
	{
		kbd_state =0;
		KeyboardRescan();
		switch (kbd_cmd)
		{//обработка событий клавиатуры
			case 0:{/*нет операции*/}break;
			case 1:{usrEvent=1;kbd_dirty=1;}break;//UP
			case 2:{usrEvent=2;kbd_dirty=1;	if (CurrentCursor==0){CurrentCursor = nCursors - 1;}else{CurrentCursor--;};}break;//LEFT
			case 3:{usrEvent=3;kbd_dirty=1;	if (CurrentCursor==nCursors-1){CurrentCursor=0;}else{CurrentCursor++;};}break;//RIGHT
			case 4:{usrEvent=4;kbd_dirty=1;}break;//DOWN
			case 5:{usrEvent=5;kbd_dirty=1;	/*выкл. сигнал при нажатии кнопки ОК если был вкл.*/ alarm_state =0; }break;//OK
			case 6:{panel_period =32; powersafe_enable =1;kbd_period = 16;/*переход на панель по умолчанию*/	NewPanel=DefaultPanel; 	CurrentCursor =0;}break;//истек таймаут клавиатуры
			default:{}break;
		};
		kbd_cmd =0;
		if (kbd_dirty){ powersafe_enable =0;panel_period =1; panel_state = panel_period+1;kbd_dirty=0;kbd_period = 0;};
	};
//--

		if (polling_state > 59)
		{//один раз в минуту //принятие решения по событию
			polling_state = 0; //запрещена реакция, будет разрешена через 1 минуту
			if (EventPolling())
			{//cобытие не обработано
					Beep();
					if (eventAttribures & 0x04)
					{ 
							alarm_state =1;										
					};
				   relay_mode_auto = eventAttribures&0x03;
				   if (relay_mode_manual == 0x03) termostat_setting = eventTemperature;	   	   	
			};//if
		continue;
		};//if

//--

	if (relay_state>16)
	{
		relay_state=0;

			switch (relay_mode_manual)
			{
				case 0x00:{//РЕЛЕ:ВЫК //стрелка вниз выключено 0xB3
					SwitchRelay (RELAY_OFF);
				}break;
				case 0x01:{//РЕЛЕ:ВКЛ //стрелка вверх включено 0xB2
					SwitchRelay (RELAY_ON);
				}break;
				case 0x02:{//РЕЛЕ:РУЧ
					SwitchRelay (Termostat());
				}break;
				case 0x03:{//РЕЛЕ:ABT
					switch (relay_mode_auto)
					{
						case 0x00:{//не может влиять на состояние реле
						}break;
						case 0x01:{//реле ВЫКЛЮЧЕНО   //стрелка вниз 0xB3
							SwitchRelay (RELAY_OFF);
						}break;
						case 0x02:{//реле ВКЛЮЧЕНО    //стрелка вверх 0xB2
							SwitchRelay (RELAY_ON);
						}break;
						case 0x03:{//режим T СТАБИЛИЗАЦИЯ ТЕМПЕРАТУРЫ
							SwitchRelay (Termostat());
						}break;
						default:{}break;					
					};
				}break;
				default:{}break;
			};
	};

	if (pwr_voltage<152)
	{
	//=======	
	if (led_state>158) {SwitchLED(2);}
	if (led_state>159) {led_state =0 ;SwitchLED(0);}
	//=======
	}
	else
	{
	//=======	
	if (led_state>158) {SwitchLED(1);}
	if (led_state>159) {led_state =0 ;SwitchLED(0);}
	//=======
	};


skip_application:	

	if (panel_state > panel_period)
		{
			panel_state=0;
			RenderPanel();    //прорисовка панели
			TIMSK = TIMSK
				 	|(1<<TOIE0)		//Overflow Interrupt Enable
					; 
			//	while (bufferSPI_complete==0);	
		};

	
//----------------------------------------------------------------------------		

	
if (powersafe_enable)
{
		if (alarm_state == 0) 
		{
			if (DurationCounter==0)
				{
					_SLEEP();
				};
		};
};
//----------------------------------------------------------------------------
	}
	while(1);
return 0;
}

unsigned char Termostat (void)
{
	signed char Tm;
	signed char Ts;
	Ts = termostat_setting;
	Tm = TLogStorage[CurrentTLogIndex];
	if (Tm < (Ts)) return RELAY_ON;
	if (Tm > (Ts)) return RELAY_OFF;
	return RELAY_KEEP;
}


//Field Number constants
#define	GFLD0		 		0x00
#define GFLD1		 		0x50
#define GFLD2		 		0xA0
#define GFLD3		 		0xF0
///////////////////////////////////////////////////////////////////////

extern const unsigned char fnt [256][6];

//void RenderChar(unsigned char X, unsigned char Y, unsigned char DrawMode,unsigned char CharCode);
void RenderChar(unsigned char X, unsigned char Y,unsigned char DrawMode, unsigned char CharCode)
{
	unsigned int index;
	unsigned int nn;
	unsigned char temp;
	unsigned char tick;
	
	while (X>12) {X -=12;};//X:0...12
	index = 2+X*6;//Y:0...4
	
	switch (Y&0x03)
	{//выбор поля индикактора (4 поля по 80x1byte)
		case 0x00: index += GFLD0;	break;  //0+080	
		case 0x01: index += GFLD1;	break;  //0+080	
		case 0x02: index += GFLD2;	break;  //0+160
		case 0x03: index += GFLD3;	break;  //0+240
		default:{}break;
	}
	tick =tick_value;
	for (nn=0;nn<6;nn++)
	{//по количеству байт символа
		if (nn<5) 
		{//при урезанном до 5ти быйт знакогенераторе	
			if (CharCode > 0x1F && CharCode < 0x41){temp=pgm_read_byte(&fnt_EN[CharCode-0x20][nn]);}
			else
			if (CharCode > 0xBF && CharCode < 0xE0){temp=pgm_read_byte(&fnt_RU[CharCode-0xC0][nn]);}
			else
			if(CharCode > 0xAF && CharCode < 0xB4){temp=pgm_read_byte(&fnt_PG[CharCode-0xB0][nn]);}
			else 
			{temp =0;};
		}
		else {temp =0;};
			
			switch (DrawMode & FLI)
			{
				case POS:{//const positive
					GBuffer[index+nn] = temp;
				}break;
				
				case NEG:{//const negative
					GBuffer[index+nn] = temp^0xFF;
				}break;

				case FLO:{//flash on/off
					if(tick){GBuffer[index+nn] =0;}
					else
					{GBuffer[index+nn] = temp;};
				}break;
				case FLI:{//flash inverse
					if(tick){GBuffer[index+nn] = temp;}
					else
					{GBuffer[index+nn] = temp^0xFF;};
				}break;
				default:{}break;
			};				
	};//for
	return;	
}

void Pixel(unsigned char X, unsigned char Y)
{//установить пиксел с координатами X Y
	unsigned char Mask;
	unsigned int Filed;
		Mask =Y & 0x07;
		Mask = 1<<Mask;	
		    switch (Y & 0x18)
			{
				case 0x00:Filed = GFLD0;break;		
				case 0x08:Filed = GFLD1;break;
				case 0x10:Filed = GFLD2;break;
				case 0x18:Filed = GFLD3;break;
				default:{return;}break;
			}		
			GBuffer[Filed+X] |=Mask;		
return;
}

///////////////////////////////////////////
extern const unsigned char days [7][2] PROGMEM;
extern const unsigned char month [12][3] PROGMEM;
extern const unsigned char *tokens [] PROGMEM;
extern const unsigned char Panels [][5] PROGMEM;

void MapPanels(void)
{//поиск доступных панелей
	unsigned char fld_type;
	unsigned char nn;
	unsigned char mm;
	nn=0;
	mm=0;
	PanelsMap[mm]=0; //0...PANELS
		do
		{
			fld_type = pgm_read_byte(&Panels[nn][0]);
			if (fld_type ==F_EPN)
			{
				PanelsMap[mm+1]=nn+1;
				mm++;
			};
			nn++;
		}while(mm<PANELS);
	return;
}

////////////////////////////////////////////////////

void RenderPanel(void)
{
//параметры поля
unsigned char fld_type;
unsigned char fld_pos;
unsigned char fld_attr;
unsigned char fld_sub;
unsigned char fld_dat;
unsigned char fld_len;
unsigned char fld_n;
//признаки
unsigned char end_panel; //конец панели
unsigned char interpret_cmd; //отработать команду
unsigned char change_panel;
//режимы рисования
unsigned char field_draw_mode=0;
unsigned char cursor_draw_mode =0;
unsigned char temp_draw_mode=0;
//временная переменная общего назначения
unsigned char temp=0;
signed char s_temp=0;
//индексы и указатели общего назначения
unsigned int nn;
unsigned char n=0;
unsigned char m;
unsigned char *b;

	if (CurrentPanel!=NewPanel)
	{//настройка на новую панель
		b = &GBuffer[0];
		for (nn=0;nn<320;nn++){*b=0x00;b++;};
		DefaultPanel=0;
		CurrentPanel = CursorBindPanel = NewPanel;
		CurrentCursor =0;
		cursors_update=1;
		nCursors=0;
		IndexesUpdate(indexCurrentRecord);
		IndexesUpdate (indexMinimorum);	
	};

//просмотр всех данных файла панелей от смещения до следующего F_EPN
	if (CurrentPanel<PANELS)
	{//панель действительна
		fld_n=PanelsMap[CurrentPanel];//смещение панели в файле панелей
			end_panel =0;
				change_panel =0;				
		do
		{
			interpret_cmd =0;
				fld_len=0;
			fld_type  = pgm_read_byte(&Panels[fld_n][0]);//тип поля
			fld_pos   = pgm_read_byte(&Panels[fld_n][1]);//размещение
			fld_attr  = pgm_read_byte(&Panels[fld_n][2]);//графические аттрибуты
			fld_sub   = pgm_read_byte(&Panels[fld_n][3]);//обработчик курсора
			fld_dat   = pgm_read_byte(&Panels[fld_n][4]);//индекс источника данных	
			
			//изменить режим рендеринга
					cursor_draw_mode =(fld_attr & 0xF0)>>4;
					field_draw_mode =fld_attr & 0x0F;			

			//изменить источник данных
					STime * p_src;
					p_src = (STime *)&date;
			b=&BCD[0];
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//секция ввывод полей			
			switch (fld_type & 0xF0)
			{
				//--
				case F_EPN:{//обнаружен конец панели
					end_panel =1;
				}break;
				//--

				case F_CMD:{//обнаружена команда
					
				interpret_cmd =fld_type;

						switch (interpret_cmd)
						{
							//установить панель перехода для курсора
							case OP(10):{CursorBindPanel= fld_dat;}break;
							//установить панель перехода по умолчанию
							case OP(11):{DefaultPanel = fld_dat;}break;

							case OP(12):{
									
									for (n=0;n<80;n++)
									{//отрисовка графика температуры
										intState = 0x80 & SREG; 	//Сохранить состояния прерываний
			    						cli ();						//Запретить прерывания.
										GBuffer[GFLD1+n]=0x00;
										GBuffer[GFLD2+n]=0x00;
										GBuffer[GFLD3+n]=0x00;
										for (m=31-((TLogStorage[n]>>1)-13);m<31;m++){Pixel (n,m);};
										Pixel (n,8);	
										Pixel (CurrentTLogIndex,9);
										Pixel (CurrentTLogIndex,10);
										Pixel (CurrentTLogIndex,11);
										SREG |= intState;			//Восстановить состояния прерываний.
									};

							}break;
							default:{}break;
						};
				}break;
				//--
				case F_YEA:{//обнаружено поле "ГОД"
					fld_len =4;
					Int2BCD2(p_src->years,fld_len);//год
				//	Int2BCD2(pwr_voltage,4);
				}break;
				//--
				case F_MON:{//обнаружено поле "МЕСЯЦ"
					for (n=0;n<3;n++){*b= pgm_read_byte(&month[p_src->months-1][n]);b++;};//месяц
					fld_len = n;
				}break;
				//--
				case F_DAY:{//обнаружено поле "ЧИСЛО"
					fld_len =2;
					Int2BCD2(p_src->days,fld_len);//день
				}break;
				//--
				case F_DOW:{//обнаружено поле "ДЕНЬ НЕДЕЛИ"
					if (fld_dat==1){temp = srRecordBuffer.dow_hours >> 5;}else{temp = p_src->dow;};						
					for (n=0;n<2;n++){*b = pgm_read_byte(&days[temp][n]);b++;};//день недели
					fld_len =n;
				}break;
				//--
				case F_HOR :{//обнаружено поле "ЧАСЫ"
					fld_len =2;
					if (fld_sub==3)
					{Int2BCD2(srRecordBuffer.dow_hours & 0x1F,fld_len);}
					else if (fld_sub==2)
					{Int2BCD2(p_src->hours,fld_len);};
				}break;
				//--
				case F_MIN:{//обнаружено поле "МИНУТЫ"
					fld_len =2;
					if (fld_dat==3)
					{Int2BCD2(srRecordBuffer.minutes,fld_len);}
					else
					{Int2BCD2(p_src->minutes,fld_len);
					}
				}break;
				//--
				case F_SEC:{//обнаружено поле "СЕКУНДЫ"
					fld_len =2;
					Int2BCD2(p_src->seconds,fld_len);
				}break;
				//--
				
				case F_TPS:{//обнаружено поле "ТЕМПЕРАТУРА"
					fld_len =5;//с дробной частью
					switch (fld_dat)
					{
						case 0:{//без дробной части
							fld_len =3;
						}	
						case 1:{//источник 1 лог температуры
							temp = TLogStorage[CurrentTLogIndex];
						}break;

						case 4:{//источник 2 буфер текущей записи
							temp = srRecordBuffer.attrib_a_pt & 0x03;
							if (temp < 0x03){skip_cursor =1;break;}
							temp = srRecordBuffer.temperature;
						}break;
						
						case 5:{//буфер текущей уставки
							skip_cursor =1;	//не активно в режимах 0-РЕЛЕ:ВЫК 1-РЕЛЕ:ВКЛ
							if ((relay_mode_manual==0x02) || ((relay_mode_manual==0x03) && (relay_mode_auto ==0x03)))
							{//активно в режимах 2-РЕЛЕ:РУЧ 3-РЕЛЕ:ABT 
								temp = termostat_setting;
								skip_cursor =0;
							};
						}break;
					};
						if (skip_cursor){*(b+0)=' ';*(b+1)='-';*(b+2)='-';*(b+3)='.';*(b+4)='-';skip_cursor=0; break;};
				//присутствует дробная часть
						*(b+3)='.';
						if (temp & 0x01){*(b+4)='5';}else{*(b+4)='0';};				
				//целое без знака - преобразовать в ASCII
						if (temp & 0x80) {
						temp=~temp;
						temp++;
						temp =temp>>1;
						Int2BCD2(temp,3);
						temp |=0x80;}
						else {Int2BCD2(temp >>1,3);};
				//знак - преобразовать в ASCII
						if (temp & 0x80) {*(b+0) ='-';}else{*(b+0)='+';};
				}break;				
				
				//--
				case F_PRI:{//обнаружено поле "ИНДЕКС ПРОГРАММЫ"
					fld_len =2;
				
				if (fld_type==F_PRP)
				{
				Int2BCD2(record_position,fld_len); //количество не стертых
				}
				else
				{
				//	Int2BCD2(indexCurrentRecord,fld_len); //текущий указатель на запись
					Int2BCD2(RecordsNumber,fld_len); //количество не стертых
				};	
				
				
				}break;
			
				//--
				case F_RCA:{//обнаружено поле "АТРИБУТ ПОДАЧИ СИГНАЛА"
					fld_len =1;
					if (srRecordBuffer.attrib_a_pt & 0x04) {*b='С';}else{*b='-';};
				}break;
				//--
				case F_RCT:{//обнаружено поле "АТРИБУТ РЕЛЕ ТЕМПЕРАТУРЫ"
					fld_len =1;
					switch (srRecordBuffer.attrib_a_pt & 0x03)
					{
						case 0x00:{*b='-';}break;
						case 0x01:{*b=0xB3;}break; //стрелка вниз выключено 0xB3
						case 0x02:{*b=0xB2;}break; //стрелка вверх включено 0xB2
						case 0x03:{*b='Т';}break;
					};
				}break;
				//--
				case F_PMM:{
						switch (relay_mode_manual)
						{
							case 0x00:{//0-РЕЛЕ:ВЫК
								fld_dat =TOKEN(0x04);
							}break;
							case 0x01:{//1-РЕЛЕ:ВКЛ
								fld_dat =TOKEN(0x03);
							}break;
							case 0x02:{//2-РЕЛЕ:РУЧ
								fld_dat =TOKEN(0x02);
							}break;
							case 0x03:{//3-РЕЛЕ:ABT
								fld_dat =TOKEN(0x01);
							}break;
						};
				}
				//---
				case F_TXT:{//обнаружено поле "ТЕКСТ/ТОКЕН" вывод текста
					unsigned char *pgm_address;
					pgm_address =(char*)pgm_read_word(&tokens[fld_dat]);
					fld_len=0;//изначально неизвестно
					do{*(b+fld_len) = pgm_read_byte (pgm_address + fld_len);fld_len++;}while (*(b+fld_len-1));
					fld_len--;//последний взятый символ /0
				}break;
				//---
				case F_PST:{//текущее состояние реле
					fld_len =1;
					if (relay_last_state==1) *(b+0)=0xB2; //стрелка вверх включено 0xB2 
					if (relay_last_state==0) *(b+0)=0xB3; //стрелка вниз выключено 0xB3	
				}break;
				//---
				default:{}break;
		 	};//switch


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//секция обработка курсора				
				if (!end_panel)
				{//признак конца панели не обнаружен
					if (interpret_cmd)
					{//интерпретация команд 
					}
					else
					{//вывод данных на экран
						if (cursors_update)
						{//проведение индексация курсоров при загрузке новой панели 	
							if (cursor_draw_mode!=POS)
							{//обнаружен курсор
								CursorsMap[nCursors] =fld_n;
								nCursors++;
							};
						}//при первом рендеринге панели	
						else
						{//второй и последующих рендеринги панели		
							temp_draw_mode = field_draw_mode;
									if (nCursors&&(CursorsMap[CurrentCursor]==fld_n))
									{//обнаружен фокус данного поля
											temp_draw_mode = cursor_draw_mode;//установка подсветки фокуса
											switch (fld_sub)
											{//то что делаем с полем при нажатии кнопок
												case 2:{/*изменить значения даты*/
												if ((usrEvent==1) || (usrEvent==4))ChangeDATE(usrEvent & 0x01,fld_dat);/*1=UP 4=DOWN*/}break;
												case 3:{/*изменить значения текущей записи eeprom */
												if ((usrEvent==1) || (usrEvent==4))ChangeERecord(usrEvent & 0x01,fld_dat);/*1=UP 4=DOWN*/}break;												
												case 4:{//перейти к другой записи
													if (usrEvent==1){/*UP*/IndexesUpdate (indexCurrentRecord);IndexesUpdate (indexStored_R);};
													if (usrEvent==4){/*DOWN*/IndexesUpdate (indexCurrentRecord);IndexesUpdate (indexStored_L);};
												}break;
												case 5:{/*добавление записи*/if (usrEvent==5) {/*OK*/ AddRecord(); CurrentCursor=0;};}break;
												case 6:{/*удаление записи*/	if (usrEvent==5){/*OK*/ DeleteCurrentRecord(); CurrentCursor=0;};}break;
												case 7:{/*изменить температуру уставки */
													s_temp = termostat_setting;
													if (usrEvent==1)//1=UP 
													{//инкремент	
														if (s_temp < 120) {s_temp++;}	 //60градусов старший бит знак  младший бит 0.5 градусов
													}
													else
													if (usrEvent==4)//4=DOWN
													{//декремент
														if (s_temp > -120) {s_temp--;};
													};
													termostat_setting =s_temp;
												}break;
											
											case 10:{/*сменить панель*/ 
													if (usrEvent==5) 
													{/*OK*/
															NewPanel=CursorBindPanel;
															usrEvent=0;
															end_panel=1;
													};
													change_panel =1;
												}break;
												case 12:{//изменение режима реле
													temp = relay_mode_manual;
													if (usrEvent==1)//1=UP 
													{//инкремент	
														if (temp < 3) {temp++;}else{temp =0;};	 
													}
													else
													if (usrEvent==4)//4=DOWN
													{//декремент
														if (temp >0) {temp--;}else{temp =3;};
													};
													relay_mode_manual =temp;
												}break;
												//default:{}break;
											};//switch
										usrEvent=0;	
									};//if //обнаружен фокус текущего поля 
						};//второй и последующих рендеринги панели
						
						
						
						{//////////////////////////////////////////////					
							b=&BCD[0];
							for (n=0;n<fld_len;n++)
							{//рендеринг поля (перенос поля в буфер экрана)
								RenderChar((fld_pos & 0x0F)+n,(fld_pos >>4)&0x03, temp_draw_mode , *b);
								b++;//к следующему
							};
						}//////////////////////////////////////////////
					
					};//вывод данных на экран
					fld_n++; //к следующему полю панели
				};//признак конца панели не обнаружен
		}
		while (!end_panel);

			if (cursors_update){cursors_update=0;};	//не применять индексацию курсоров в дальнейшем		
		
				if (change_panel==0)
				{
					if (usrEvent==5) {NewPanel=CursorBindPanel;usrEvent=0;};			
				};
	};//if ошибка номера панели
return;
}








