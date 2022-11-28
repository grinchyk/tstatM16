#include <avr\io.h>
#include <avr\interrupt.h> 
#include "one_wire.h"

/////////////////////////////////////////////////////////////////////////////
unsigned char intState;
/////////////////////////////////////////////////////////////////////////////

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
/////////////////////////////////////////////////////////////////////////////
//специальные операции с шиной 1Wire
/////////////////////////////////////////////////////////////////////////////////////////////////////
//One Wire BUS
			unsigned char OneWire_TestPresence(void);//__attribute__ ((naked));		
	 		unsigned char OneWire_TestPresence(void)
			{//определение наличи€ устройств подключенных к шине 
				unsigned char Presence=0;
				intState = 0x80 & SREG; 							//—охранить состо€ни€ прерываний
			    cli ();												//«апретить прерывани€. 			
				OW_DDR &= ~OW_MASK;  								//освободить линию (1 на подт€жке)
				OW_PORT &= ~OW_MASK; 								//подготовить уровень 0 в порт
			    _DELAY_NCLK(OW_DELAY_T_REC);						//предварительна€ задержка recovery 
//RESET PULSE.     
				OW_DDR |=OW_MASK; 									//установить на линии 0
				_DELAY_NCLK(OW_DELAY_T_RSTL);						//интервал мин 480us 
				OW_DDR &= ~OW_MASK; 						    	//освободить линию (1 на подт€жке)
//DETECT PRESENCE 
				_DELAY_NCLK(120);//52-249 //120 -устойчиво
				Presence = ((~OW_PIN) & OW_MASK);
				_DELAY_NCLK(OW_DELAY_T_RSTH);	//завершить таймслот мин240us
			    			
				SREG |= intState;								//¬осстановить состо€ни€ прерываний.			  
				return Presence;
			}



/////////////////////////////////////////////////////////////////////////////
//битовые операции с шиной 1Wire
void OneWire_Write_Bit(unsigned char BitValue);//__attribute__ ((naked));	
void OneWire_Write_Bit(unsigned char BitValue)
			 {
				intState = 0x80 & SREG; 								//—охранить состо€ни€ прерываний
			    cli ();													//«апретить прерывани€.
				
				OW_DDR &= ~OW_MASK;  							//освободить линию (1 на подт€жке)
				OW_PORT &= ~OW_MASK; 							//подготовить уровень 0 в порт
			    _DELAY_NCLK(10);						    //OW_DELAY_T_REC предварительна€ задержка recovery 
			    
				if (BitValue == 0) 
			    {//выдержать интервалы дл€ передачи 0
				        OW_DDR |= OW_MASK; 						//установить на линии 0
				        _DELAY_NCLK(80); 					//OW_DELAY_T_LOW0 интервал ~67.5us дл€ 0
						OW_DDR &= ~OW_MASK;						//освободить линию (1 на подт€жке)
						_DELAY_NCLK(40);	//OW_DELAY_T_SLOT-OW_DELAY_T_LOW0 интервал 10us recovery time
				}
			    else 
			    {//выдержать интервалы дл€ передачи 1 
						OW_DDR |= OW_MASK;						//установить на линии 0
						_DELAY_NCLK(7); 					//OW_DELAY_T_LOW1 интервал ~6,75us < 15us дл€ 1 
					    OW_DDR &= ~OW_MASK;						//освободить линию
					    _DELAY_NCLK(110);	//OW_DELAY_T_SLOT-OW_DELAY_T_LOW1 завершить timeslot 10us recovery 
				};
				SREG |= intState;										//¬осстановить состо€ни€ прерываний.
			 }

///////////////////////////////////////////////////////////////////////////
//битовые операции с шиной 1Wire
unsigned char OneWire_Read_Bit(void);//__attribute__ ((naked));
unsigned char OneWire_Read_Bit(void)
			{
				intState = 0x80 & SREG; 							//—охранить состо€ни€ прерываний
			    cli ();												//«апретить прерывани€.
				
				unsigned char BitValue;
				OW_DDR &= ~OW_MASK;  						//освободить линию (1 на подт€жке)
				OW_PORT &= ~OW_MASK; 						//подготовить уровень 0 в порт
			    _DELAY_NCLK(10);						//OW_DELAY_T_REC предварительна€ задержка recovery

					OW_DDR |= OW_MASK; 						//установить на линии 0
					_DELAY_NCLK(7);					//OW_DELAY_T_RDV_1 интервал 6us.
			
					OW_DDR &= ~OW_MASK;						//освободить линию (1 на подт€жке) 
					_DELAY_NCLK(7);					//OW_DELAY_T_RDV_2 интервал 7us

					if (OW_PIN & OW_MASK){BitValue=0x80;}else{BitValue =0;};//прочитать уровень шины
					_DELAY_NCLK(120);				//OW_DELAY_T_SLOT-OW_DELAY_T_RDV завершить timeslot
					
				SREG |= intState;									//¬осстановить состо€ни€ прерываний.
			    return BitValue;
			}


////////////////////////////////////////////////////////////////////////////
//байтовые операции с шиной 1Wire
			void OneWire_Send_Byte(unsigned char Data);
			void OneWire_Send_Byte(unsigned char Data) 
				 {//отправка 8 бит
				    unsigned char i;
					for (i=0;i<8;i++)
					 { 
				         OneWire_Write_Bit(Data & 0x01); 
				         Data = Data >> 1; 							 
				     };
					 return;
				 }
			
			unsigned char OneWire_Recieve_Byte(void); 
			unsigned char OneWire_Recieve_Byte(void) 
				 {//прием 8 бит
				    unsigned char rcv =0;
					unsigned char i;
				    for (i = 0; i < 8; i++)
					{
						rcv = rcv>>1;
						rcv |= OneWire_Read_Bit(); 
				    };
				    return rcv; 
				 }

/////////////////////////////////////////////////////////////////////////////////////////////////////
//One Wire BUS

/////////////////////////////////////////////////////////////////////////////////////////////////////
//DS1820

//void OneWire_DS1820_Read_ScratchPad(void);
//void OneWire_DS1820_ClaimT(void);
