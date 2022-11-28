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
		while (EECR & (1<<EEWE));	// готовность eeprom
		EEAR = address;					// загрузка адреса
		EECR |= (1<<EERE);				// режим чтение eeprom
		if (EEDR == (*buffer))
		 {
		 	address++;
		buffer++;
		continue;};		// нет изменений не будем писать

		pp=*buffer;
		EEDR = pp;					// загрузка данных для записи
		intState = 0x80 & SREG; 		//Сохранить состояния прерываний
		cli ();							//Запретить прерывания.
			EECR |= (1<<EEMWE);		// режим запись 
			EECR |= (1<<EEWE);			// произвести запись
		SREG |= intState;				//Восстановить состояния прерываний.
		address++;
		buffer++;
	};
}

//EEPROM
void eepromReadBuffer(unsigned int address, unsigned char *buffer, unsigned char length);
void eepromReadBuffer(unsigned int address, unsigned char *buffer, unsigned char length)
{
	unsigned char i;
	while (EECR & (1<<EEWE));		// готовность eeprom
	for (i=0; i<length; i++)
	{
		EEAR = address;				// загрузка адреса
		EECR |= (1<<EERE);				// режим чтение eeprom
		*buffer = EEDR;				// вернуть байт
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
