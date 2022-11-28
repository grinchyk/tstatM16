#include <avr\io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <avr\interrupt.h> 
#include "clock.h"
/////////////////////////////////////////////////////////////////////////////
unsigned char intState;
/////////////////////////////////////////////////////////////////////////////

void TimeINC(void);
void UpdateMonthDays(void);
void UpdateBissextile(void);
void UpdateDOW(void);
void ClockReset(void);// __attribute__ ((naked));

//количество дней в месяцах
const const unsigned char mdays[12] PROGMEM={31,28,31,30,31,30,31,31,30,31,30,31};
//сокращения названий месяцев
const const unsigned char month [12][3] PROGMEM=
{
{0xDF,0xCD,0xC2},//ЯНВ
{0xD4,0xC5,0xC2},//ФЕВ
{0xCC,0xC0,0xD0},//МАР
{0xC0,0xCF,0xD0},//АПР
{0xCC,0xC0,0xC9},//МАЙ
{0xC8,0xDE,0xCD},//ИЮН
{0xC8,0xDE,0xCB},//ИЮЛ
{0xC0,0xC2,0xC3},//АВГ
{0xD1,0xC5,0xCD},//СЕН
{0xCE,0xCA,0xD2},//ОКТ
{0xCD,0xCE,0xDF},//НОЯ
{0xC4,0xC5,0xCA},//ДЕК
};
//сокращения названий дней недели
const const unsigned char days [7][2] PROGMEM=
{
{0xC2,0xD1},//ВС
{0xCF,0xCD},//ПН
{0xC2,0xD2},//ВТ
{0xD1,0xD0},//СР
{0xD7,0xD2},//ЧТ
{0xCF,0xD2},//ПТ
{0xD1,0xC1} //СБ
};


//////////////////////////////////////////////////////////////////////////////
//clock.c
STime date;
void ClockReset(void);
void TimeINC(void);
//////////////////////////////////////////////////////////////////////////////

//--
void SaveRecord (unsigned char index);
void LoadRecord (unsigned char index);
void eepromReadBuffer(unsigned int address, unsigned char *buffer, unsigned char length);
void eepromWriteBuffer(unsigned int address, unsigned char *buffer, unsigned char length);
void eepromClear(void);
void IncERecord (unsigned char index);
void DecERecord (unsigned char index);
unsigned char EventsCompare (SRecord * A,SRecord *B);
unsigned char IndexesUpdate (unsigned char indexSource);
unsigned char DeleteCurrentRecord(void);
unsigned char AddRecord(void);
unsigned char EventPolling(void);
//////////////////////////////////////////////////////////////////////////////////////////
unsigned char 	eventTemperature;
unsigned char	eventAttribures;
unsigned char   disable_event_on_current;

//////////////////////////////////////////////////////////////////////////////////////////
//eeprom.c
extern void eepromClear(void);
extern void eepromWriteBuffer(unsigned int address, unsigned char *buffer, unsigned char length);
extern void eepromReadBuffer(unsigned int address, unsigned char *buffer, unsigned char length);
/////////////////////////////////////////////////////////////////////////////////////////

//--
SRecord srRecordBuffer; //буфер хранения записи для редактирования и вывода на экран
unsigned char indexCurrentRecord; //индекс текущей отображаемой записи (которая хранится в RecordBuffer)
unsigned char RecordsNumber;//количество не стертых записей в ееprоm
unsigned char indexLastDeleted;//индекс первой удаленной записи в списке



unsigned char indexMinimorum;
unsigned char indexStored_L;
unsigned char indexStored_R;
unsigned char indexMaximorum;


SRecord srTime;

//void EraseRecord(unsigned char index);
void EraseRecord(unsigned char index)
{
	unsigned char icnt;
	unsigned char SingleByte;
	unsigned int eepromAddress;
	SingleByte=0x00;
	eepromAddress = (index << 2)+1;
	for (icnt=0;icnt<4;icnt++)
	{
		eepromWriteBuffer(eepromAddress+icnt, (unsigned char*)(&SingleByte), 1);
	};
	return;
}
/////////////////////////////////////////////////////////////////////////////////////////////

//void LoadRecord (unsigned char index);
void LoadRecord (unsigned char index)
{
	unsigned int eepromAddress;
	eepromAddress = (index << 2)+1;
	eepromReadBuffer(eepromAddress, (unsigned char*)(&srRecordBuffer), 4);
	return;
}


//void SaveRecord (unsigned char index);
void SaveRecord (unsigned char index)
{
	unsigned int eepromAddress;
	eepromAddress = (index << 2)+1;
	eepromWriteBuffer(eepromAddress, (unsigned char*)(&srRecordBuffer), 4);
	return;
}

unsigned char record_position;


void IncRecDat (SRecord * RecDat)
{
unsigned char dow;
unsigned char hours;
unsigned char minutes;

dow = RecDat->dow_hours>>5;
hours = RecDat->dow_hours & 0x1F;
minutes = RecDat->minutes;

if (minutes>58)
{ 
	minutes =0;
	if (hours>22)
	{
		hours =0;
		if (dow>5)
		{
			dow =0;
		}
		else
		{
			dow++;
		};
	}
	else
	{
		hours++;
	};
}
else
{
	minutes++;
};

RecDat->dow_hours = dow<<5;
RecDat->dow_hours |=hours;
RecDat->minutes =minutes;

return;
}

unsigned char indexLastAdded;
unsigned char AddRecord(void)
{
	unsigned int eepromAddr;
	SRecord srNext;
	if (indexLastDeleted<99)     ///!!!
	{
			//заготовить чистую запись 
			//--
			IndexesUpdate(indexLastAdded);
			srNext = srRecordBuffer;
			IncRecDat ( &srNext );
			srNext.temperature = 0x25;//18.5
			srNext.attrib_a_pt =0x80;//отметка о неудаленной записи
			//записать новую запись
			eepromAddr = (indexLastDeleted * 4)+1;
			eepromWriteBuffer(eepromAddr, (unsigned char*)(&srNext), 4);	
			//присвоить новую запись буферу
			srRecordBuffer = srNext;
			indexLastAdded = indexLastDeleted;
			IndexesUpdate(indexLastDeleted);//временно установить текущей для редактирования
			disable_event_on_current =1;
			record_position =RecordsNumber;
			return 0x00;
	}
	else 
	{/*добавление невозможно - список заполнен на 100%*/};
return 0;
}

unsigned char DeleteCurrentRecord(void)
{
	EraseRecord(indexCurrentRecord);	//очистка памяти записи
	if (IndexesUpdate(indexCurrentRecord)==0xFF)
	{
		//справочник пуст
		AddRecord();//автоматически добавить одну новую запись для шаблона
	};
	return 0x00;
}


unsigned char EventsCompare (SRecord * A,SRecord *B)
{
	unsigned char dow_A,dow_B,hours_A,hours_B,minutes_A,minutes_B;
	dow_A=A->dow_hours & 0xE0;
	dow_B=B->dow_hours & 0xE0;
	hours_A=A->dow_hours & 0x1F;
	hours_B=B->dow_hours & 0x1F;
	minutes_A=A->minutes & 0x3F;
	minutes_B=B->minutes & 0x3F;

		if ((dow_A)<(dow_B)){return 0x01;} else
		if ((dow_A)>(dow_B)){return 0xFF;} else
		if ((dow_A)==(dow_B))
		{
			if ((hours_A)<(hours_B)){return 0x01;} else
			if ((hours_A)>(hours_B)){return 0xFF;} else
			if ((hours_A)==(hours_B))
			{
				if ((minutes_A)<(minutes_B)){return 0x01;} else
				if ((minutes_A)>(minutes_B)){return 0xFF;} else
				if ((minutes_A)==(minutes_B)){return 0x00;};
			};	
		};
	return 0x00;
}


unsigned char EventPolling(void)
{

unsigned int eeprAddr;	
unsigned char indexNext;
SRecord srNext;
//выборка текущего времени //преобразование к формату справочника
srTime.dow_hours = date.dow <<5; 
srTime.dow_hours +=date.hours;
srTime.minutes = date.minutes;

for (indexNext=0;indexNext<99;indexNext++)
	{
	//чтение следующего NEXT
	if (disable_event_on_current && (indexNext==indexCurrentRecord)) continue;
	eeprAddr = (indexNext * 4)+1;
	eepromReadBuffer(eeprAddr, (unsigned char*)(&srNext), 4);
		if (srNext.attrib_a_pt & 0x80)
		{//запись не стерта	
			if (EventsCompare (&srNext,&srTime)== 0x00)
			{//поллинг текущего времени
				eventTemperature = srNext.temperature;
				eventAttribures = srNext.attrib_a_pt;
				return 0x01;
			};
		};//if
	};//for
return 0x00;
}




unsigned char IndexesUpdate (unsigned char indexSource)
{
//обновление индексов 
unsigned int eeprAddr;	
unsigned char indexNext;
unsigned char kill_duplicates=0;
SRecord srStored_L;
SRecord srStored_R;
SRecord srMaximorum;
SRecord srMinimorum;
SRecord srSource;
SRecord srNext;
//---------------------------------------------------------------------
	record_position=1;
	
	indexNext = indexSource;
	do
	{
	//расчет адреса источника
	eeprAddr = (indexNext * 4)+1;
	//загрузка данных источника
	eepromReadBuffer(eeprAddr, (unsigned char*)(&srSource), 4);
	if ((srSource.attrib_a_pt & 0x80) != 0) 
	{
	break;
	}

	//если запись стерта - поиск ближайшего существующего индекса
		if (indexNext>indexSource)
		{//продолжаем поиск вверх
			if (indexNext>98)
			{//окончить поиск - сверху записи не найдены
				//нет записей в справочнике
				indexLastDeleted =98;//справочник растет вниз по 4 байта на запись
				RecordsNumber =0;
				return 0xFF;		
			}
			else
			{//продолжаем поиск вверх
			 indexNext++;
			};
		}
		else
		{//сначала поиск вниз
			if (indexNext==0) 
			{//окончить поиск вниз, приступить к поиску вверх
				indexNext=indexSource +1;}
			else 
			{//продолжаем поиск вниз
				indexNext--;
			};
		};
	}while  (1);
//----------------------------------------------------------------------
	
disable_event_on_current =0;
//if (indexCurrentRecord != indexSource)
if (indexLastDeleted != indexSource)
{//запрос на переход к другой записи
	kill_duplicates=1;
};




indexSource = indexNext;
indexCurrentRecord = indexSource;
srRecordBuffer = srSource;
srMinimorum = srSource;
indexMinimorum= indexSource;
srMaximorum = srSource;
indexMaximorum = indexSource;
//------------------------

	indexLastDeleted =99;
	RecordsNumber =0;
	//обновление индексов проход 1 поиск экстремальных
	for (indexNext=0;indexNext<99;indexNext++)
	{
	//чтение следующего NEXT
	eeprAddr = (indexNext * 4)+1;
	eepromReadBuffer(eeprAddr, (unsigned char*)(&srNext), 4);

		if (srNext.attrib_a_pt & 0x80)
		{//запись не стерта	
			RecordsNumber++; //считаем не стертые записи
			
			switch (EventsCompare (&srNext,&srMinimorum))
			{//сравнение NEXT и MINIMORUM
				
				case 0x01:{//NEXT < MINIMORUM
					srMinimorum = srNext;
					indexMinimorum = indexNext;
				}break;

				default:{
					switch (EventsCompare (&srNext,&srMaximorum))
					{//сравнение NEXT и MAXIMORUM
						case 0xFF:{//NEXT>MINIMORUM
							srMaximorum = srNext;
							indexMaximorum = indexNext;



						}break;

					};

				}break;
			};//switch
		}
		else
		{//запись стерта
			indexLastDeleted = indexNext;
		};//if
	};//for

srStored_L = srMinimorum;
indexStored_L =indexMinimorum;
srStored_R = srMaximorum;
indexStored_R =indexMaximorum;	

	//обновление индексов проход 2 поиск максимально приближенных
	for (indexNext=0;indexNext<99;indexNext++)
	{
	//чтение следующего NEXT
	eeprAddr = (indexNext * 4)+1;
	eepromReadBuffer(eeprAddr, (unsigned char*)(&srNext), 4);
		
		//обновление индексов
		if (srNext.attrib_a_pt & 0x80)
		{//запись не стерта		
			switch (EventsCompare (&srNext,&srSource))
			{//сравнение NEXT и SOURCE
				case 0x01:{//NEXT < SOURCE

					record_position++;					

					switch (EventsCompare (&srNext,&srStored_L))
					{//сравнение NEXT и STORED_L
						case 0xFF:{//NEXT>STORED_L
							srStored_L = srNext;
							indexStored_L = indexNext;
						}break;
					};//switch
				

				}break;

				case 0xFF:{//NEXT > SOURCE
					switch (EventsCompare (&srNext,&srStored_R))
					{//сравнение NEXT и STORED_R
						case 0x01:{//NEXT<STORED_R
							srStored_R = srNext;
							indexStored_R = indexNext;
						}break;
					};
				}break;

				case 0x00:{//NEXT == SOURCE
					if (indexSource != indexNext)
					{
						if (kill_duplicates)
						{
						RecordsNumber--;
						indexLastDeleted = indexNext;
						EraseRecord(indexNext);
						};
					};
				}break;

			};//switch
		};//if
	};//for	
	return 0x00;
}










extern unsigned char CurrentTLogIndex;
extern unsigned char TLogStorage[];


void ChangeERecord(unsigned char change_mode,unsigned char index)
{
unsigned char u_temp;
signed char s_temp;
//change_mode 0-декремент 1-инкремент
	switch (index)
	{
		//--
		case 1:{//изменение DOW (.dow_hours)
			u_temp = srRecordBuffer.dow_hours & 0xE0;
			if (change_mode)
			{//инкремент
				if (u_temp == 0xC0) {u_temp=0;}else {u_temp +=0x20;};
			}
			else
			{//декремент
				if (u_temp == 0x00) {u_temp=0xC0;}else{u_temp -=0x20;};
			};
			srRecordBuffer.dow_hours &=(~0xE0); 
			srRecordBuffer.dow_hours |= u_temp;	
		}break;
		//--
		case 2:{//изменение HOURS (.dow_hours)

			u_temp = srRecordBuffer.dow_hours & 0x1F; 
			if (change_mode)
			{//инкремент
				if (u_temp < 23) {u_temp++;}else {u_temp=0;};
			}
			else
			{//декремент
				if (u_temp > 0) {u_temp--;}else {u_temp=23;};
			};
			srRecordBuffer.dow_hours &=~(0x1F);
			srRecordBuffer.dow_hours |=u_temp;
		}break;
		//--
		case 3:{//изменение MINUTES (.minutes)
			u_temp = srRecordBuffer.minutes; 
			if (change_mode)
			{//инкремент
				if (u_temp < 59) {u_temp++;}else {u_temp=0;};	
			}
			else
			{//декремент
				if (u_temp > 0) {u_temp--;}else {u_temp=59;};
			}; 
			srRecordBuffer.minutes = u_temp;
		}break;
		//--
		case 4:{//изменение температуры уставки 
			s_temp = srRecordBuffer.temperature;
			if (change_mode)
			{//инкремент	
				if (s_temp < 120) {s_temp++;}	 //60градусов старший бит знак  младший бит 0.5 градусов
			}
			else
			{//декремент
				if (s_temp > -120) {s_temp--;};
			};
			srRecordBuffer.temperature =s_temp;
		}break;
		//--
		case 5:{//циклически установка флага подачи сигнала (alarm)
			srRecordBuffer.attrib_a_pt ^=0x04;
		}break;
		//--
		case 6:{//циклически установка флагов режима реле (реле включения / реле температуры) P T
				//00-ВЫКЛ //01- РЕЖИМ РЕЛЕ ВКЛ //10-РЕЖИМ РЕЛЕ ВЫКЛ //11-СТАБИЛИЗАЦИЯ ТЕМПЕРАТУРЫ  
			u_temp = srRecordBuffer.attrib_a_pt & 0x03; 
			if (u_temp < 3) {u_temp++;}else {u_temp=0;};
			srRecordBuffer.attrib_a_pt &=(~0x03); 
			srRecordBuffer.attrib_a_pt |= u_temp;
			if (u_temp == 0x03) {srRecordBuffer.temperature = 0x25;/*18.5*/}
			else {srRecordBuffer.temperature = 0;};
		}break;
	};//switch
		//сохранение записи
		SaveRecord (indexCurrentRecord);
		LoadRecord (indexCurrentRecord);
}



/////////////////////////////////////////////////////////////////////////////
//корректировка особенностей календаря
void UpdateBissextile(void)
{//Обновить признак високосности года (bissextile)
//60 байт
	date.bissextile = 0;
	if ((date.years%4 == 0) && ((date.years % 100 != 0) || (date.years % 400 == 0))){date.bissextile =1;};
}

void UpdateMonthDays(void)
{
	date.mdays= pgm_read_byte(&mdays[date.months-1]);	
	if (date.months ==2) {date.mdays = date.mdays + date.bissextile;};
}


void UpdateDOW(void)
{//Обновить день недели 0:вс 1:пн 2:вт 3:ср 4:чт 5:пт 6:cб
//184 байт
	unsigned char month;
	unsigned int year;
	unsigned char temp;
	temp = ((14 - date.months) / 12);
	year = date.years - temp;
	month = date.months + 12 * temp - 2;
	date.dow = (7000 + (date.days + year + year / 4 - year / 100 + year / 400 + (31 * month) / 12)) % 7;
}

void ClockReset(void)
{
	date.hours=12;
	date.minutes=00;
	date.days=28;
	date.months =02;
	date.years =2007;
	UpdateBissextile();
	UpdateMonthDays();
	UpdateDOW();
	//--
	return;
}

void TimeINC(void)
{
	{//приращение секунд
		date.seconds++;
		if (date.seconds > 59)
		{
			date.seconds =0;
				{//приращение минут
					date.minutes++;
					if (date.minutes>59)
					{
						date.minutes=0;
							{//приращение часов
								date.hours++;
								if (date.hours>23)
								{
									date.hours=0;
										{//приращение дня			
												date.dow++;
												if (date.dow>6)
												{//корректировка дня недели
													date.dow=0;
												};
											date.days++;
											if (date.mdays<date.days)
											{
												date.days =1;
					{//приращение месяца---------------------------------			
						date.months++;
						if (date.months > 12)
						{
							date.months =1;
							{//приращение года
								date.years++;		
								UpdateBissextile();//Год изменен,обновление bissextile после изменения года		
							}//приращение года	
						};
						UpdateMonthDays();
					}//приращение месяца--------------------------------------
											}			 		
										}//приращение дня
								};

							}//приращение часов

					};
				}//приращение минут
		};
	}//приращение секунд
return;
}





void ChangeDATE (unsigned char change_mode,unsigned char index);
void ChangeDATE (unsigned char change_mode,unsigned char index)
{
	switch(index)
	{
	case 0:{//сброс секунд
		date.seconds =0;
	}break;
	//--
	case 1:{//изменение минут
		if (change_mode)
		{//инкремент
			if (date.minutes < 59) {date.minutes++;}else {date.minutes =0;}
		}
		else
		{//декремент
			if (date.minutes ==0) {date.minutes =59;}else{date.minutes--;};
		};
	}break;
	//--
	case 2:{//изменение часов
		if (change_mode)
		{//инкремент	
			if (date.hours < 23){date.hours ++;}else{date.hours =0;};
		}
		else
		{//декремент
			if (date.hours ==0){date.hours =23;}else{date.hours --;};
		};
	}break;
	//--
	case 3:{//изменение даты и дня недели
		if (change_mode)
		{//инкремент
			if (date.days <date.mdays){date.days++;}else{ date.days=1;};
			if(date.dow < 6){date.dow++;}else{date.dow=0;};
		}
		else
		{//декремент
			if (date.days ==1){date.days = date.mdays;}else{date.days--;};
			if(date.dow == 0){date.dow = 6;}else{date.dow--;};
		};
	}break;
	//--
	case 4:{//изменение месяца
		if (change_mode)
		{//инкремент	
			if(date.months<12){date.months++;}else{date.months=1;};
		}
		else
		{//декремент
			if(date.months==1){date.months=12;}else{date.months--;};
		};
		UpdateMonthDays();
		if (date.days>date.mdays) date.days = date.mdays;
		UpdateDOW();
	}break;
	//--
	case 5:{//изменение года
		if (change_mode)
		{//инкремент
			date.years++;
		}
		else
		{//декремент
			date.years--;
		};
		UpdateBissextile();
		UpdateMonthDays();
		UpdateDOW();
	}break;
	//--
	default:{}break;
	};
};





