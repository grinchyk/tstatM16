#include <avr\io.h>
#include <avr\interrupt.h> 
/////////////////////////////////////////////////////////////////////////////
unsigned char intState;
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//EEPROM

void eepromWriteBuffer(unsigned int address, unsigned char *buffer, unsigned char length);
void eepromWriteBuffer(unsigned int address, unsigned char *buffer, unsigned char length)
{
	unsigned char i;
	unsigned char pp;
	for (i=0; i<length; i++) 
	{
		while (EECR & (1<<EEWE));	// ���������� eeprom
		EEAR = address;					// �������� ������
		EECR |= (1<<EERE);				// ����� ������ eeprom
		if (EEDR == (*buffer))
		 {
		 	address++;
		buffer++;
		continue;};		// ��� ��������� �� ����� ������

		pp=*buffer;
		EEDR = pp;					// �������� ������ ��� ������
		intState = 0x80 & SREG; 		//��������� ��������� ����������
		cli ();							//��������� ����������.
			EECR |= (1<<EEMWE);		// ����� ������ 
			EECR |= (1<<EEWE);			// ���������� ������
		SREG |= intState;				//������������ ��������� ����������.
		address++;
		buffer++;
	};
}

//EEPROM
void eepromReadBuffer(unsigned int address, unsigned char *buffer, unsigned char length);
void eepromReadBuffer(unsigned int address, unsigned char *buffer, unsigned char length)
{
	unsigned char i;
	while (EECR & (1<<EEWE));		// ���������� eeprom
	for (i=0; i<length; i++)
	{
		EEAR = address;				// �������� ������
		EECR |= (1<<EERE);				// ����� ������ eeprom
		*buffer = EEDR;				// ������� ����
		address++;
		buffer++;
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////
void eepromClear(void);
void eepromClear(void)
{
	unsigned int eepromAddress;
	unsigned char SingleByte;
	SingleByte=0x00;
	for (eepromAddress=0;eepromAddress<512;eepromAddress++)
	{
		eepromWriteBuffer(eepromAddress, (unsigned char*)(&SingleByte), 1);
	};
	return;
};
