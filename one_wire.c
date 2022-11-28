#include <avr\io.h>
#include <avr\interrupt.h> 
#include "one_wire.h"

/////////////////////////////////////////////////////////////////////////////
unsigned char intState;
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//��������������� ���������
static  void _DELAY_NCLK(short __count) __attribute__((always_inline));
static  void _DELAY_NCLK(short __count)
{//������� ����� ��������
__asm__ __volatile__ 
(
		"L_loop%=: 		sbiw 	r24,1 				\n\t" //2clk (0.25��� ��� F_CPU=8MHz
		"				brne 	L_loop%=			\n\t" //2clk  	
://output operand list
://input operand list
//://[clobber list]						
);														  
return;
}
/////////////////////////////////////////////////////////////////////////////
//����������� �������� � ����� 1Wire
/////////////////////////////////////////////////////////////////////////////////////////////////////
//One Wire BUS
			unsigned char OneWire_TestPresence(void);//__attribute__ ((naked));		
	 		unsigned char OneWire_TestPresence(void)
			{//����������� ������� ��������� ������������ � ���� 
				unsigned char Presence=0;
				intState = 0x80 & SREG; 							//��������� ��������� ����������
			    cli ();												//��������� ����������. 			
				OW_DDR &= ~OW_MASK;  								//���������� ����� (1 �� ��������)
				OW_PORT &= ~OW_MASK; 								//����������� ������� 0 � ����
			    _DELAY_NCLK(OW_DELAY_T_REC);						//��������������� �������� recovery 
//RESET PULSE.     
				OW_DDR |=OW_MASK; 									//���������� �� ����� 0
				_DELAY_NCLK(OW_DELAY_T_RSTL);						//�������� ��� 480us 
				OW_DDR &= ~OW_MASK; 						    	//���������� ����� (1 �� ��������)
//DETECT PRESENCE 
				_DELAY_NCLK(120);//52-249 //120 -���������
				Presence = ((~OW_PIN) & OW_MASK);
				_DELAY_NCLK(OW_DELAY_T_RSTH);	//��������� �������� ���240us
			    			
				SREG |= intState;								//������������ ��������� ����������.			  
				return Presence;
			}



/////////////////////////////////////////////////////////////////////////////
//������� �������� � ����� 1Wire
void OneWire_Write_Bit(unsigned char BitValue);//__attribute__ ((naked));	
void OneWire_Write_Bit(unsigned char BitValue)
			 {
				intState = 0x80 & SREG; 								//��������� ��������� ����������
			    cli ();													//��������� ����������.
				
				OW_DDR &= ~OW_MASK;  							//���������� ����� (1 �� ��������)
				OW_PORT &= ~OW_MASK; 							//����������� ������� 0 � ����
			    _DELAY_NCLK(10);						    //OW_DELAY_T_REC ��������������� �������� recovery 
			    
				if (BitValue == 0) 
			    {//��������� ��������� ��� �������� 0
				        OW_DDR |= OW_MASK; 						//���������� �� ����� 0
				        _DELAY_NCLK(80); 					//OW_DELAY_T_LOW0 �������� ~67.5us ��� 0
						OW_DDR &= ~OW_MASK;						//���������� ����� (1 �� ��������)
						_DELAY_NCLK(40);	//OW_DELAY_T_SLOT-OW_DELAY_T_LOW0 �������� 10us recovery time
				}
			    else 
			    {//��������� ��������� ��� �������� 1 
						OW_DDR |= OW_MASK;						//���������� �� ����� 0
						_DELAY_NCLK(7); 					//OW_DELAY_T_LOW1 �������� ~6,75us < 15us ��� 1 
					    OW_DDR &= ~OW_MASK;						//���������� �����
					    _DELAY_NCLK(110);	//OW_DELAY_T_SLOT-OW_DELAY_T_LOW1 ��������� timeslot 10us recovery 
				};
				SREG |= intState;										//������������ ��������� ����������.
			 }

///////////////////////////////////////////////////////////////////////////
//������� �������� � ����� 1Wire
unsigned char OneWire_Read_Bit(void);//__attribute__ ((naked));
unsigned char OneWire_Read_Bit(void)
			{
				intState = 0x80 & SREG; 							//��������� ��������� ����������
			    cli ();												//��������� ����������.
				
				unsigned char BitValue;
				OW_DDR &= ~OW_MASK;  						//���������� ����� (1 �� ��������)
				OW_PORT &= ~OW_MASK; 						//����������� ������� 0 � ����
			    _DELAY_NCLK(10);						//OW_DELAY_T_REC ��������������� �������� recovery

					OW_DDR |= OW_MASK; 						//���������� �� ����� 0
					_DELAY_NCLK(7);					//OW_DELAY_T_RDV_1 �������� 6us.
			
					OW_DDR &= ~OW_MASK;						//���������� ����� (1 �� ��������) 
					_DELAY_NCLK(7);					//OW_DELAY_T_RDV_2 �������� 7us

					if (OW_PIN & OW_MASK){BitValue=0x80;}else{BitValue =0;};//��������� ������� ����
					_DELAY_NCLK(120);				//OW_DELAY_T_SLOT-OW_DELAY_T_RDV ��������� timeslot
					
				SREG |= intState;									//������������ ��������� ����������.
			    return BitValue;
			}


////////////////////////////////////////////////////////////////////////////
//�������� �������� � ����� 1Wire
			void OneWire_Send_Byte(unsigned char Data);
			void OneWire_Send_Byte(unsigned char Data) 
				 {//�������� 8 ���
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
				 {//����� 8 ���
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
