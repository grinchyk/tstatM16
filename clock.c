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

//���������� ���� � �������
const const unsigned char mdays[12] PROGMEM={31,28,31,30,31,30,31,31,30,31,30,31};
//���������� �������� �������
const const unsigned char month [12][3] PROGMEM=
{
{0xDF,0xCD,0xC2},//���
{0xD4,0xC5,0xC2},//���
{0xCC,0xC0,0xD0},//���
{0xC0,0xCF,0xD0},//���
{0xCC,0xC0,0xC9},//���
{0xC8,0xDE,0xCD},//���
{0xC8,0xDE,0xCB},//���
{0xC0,0xC2,0xC3},//���
{0xD1,0xC5,0xCD},//���
{0xCE,0xCA,0xD2},//���
{0xCD,0xCE,0xDF},//���
{0xC4,0xC5,0xCA},//���
};
//���������� �������� ���� ������
const const unsigned char days [7][2] PROGMEM=
{
{0xC2,0xD1},//��
{0xCF,0xCD},//��
{0xC2,0xD2},//��
{0xD1,0xD0},//��
{0xD7,0xD2},//��
{0xCF,0xD2},//��
{0xD1,0xC1} //��
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
SRecord srRecordBuffer; //����� �������� ������ ��� �������������� � ������ �� �����
unsigned char indexCurrentRecord; //������ ������� ������������ ������ (������� �������� � RecordBuffer)
unsigned char RecordsNumber;//���������� �� ������� ������� � ��pr�m
unsigned char indexLastDeleted;//������ ������ ��������� ������ � ������



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
			//���������� ������ ������ 
			//--
			IndexesUpdate(indexLastAdded);
			srNext = srRecordBuffer;
			IncRecDat ( &srNext );
			srNext.temperature = 0x25;//18.5
			srNext.attrib_a_pt =0x80;//������� � ����������� ������
			//�������� ����� ������
			eepromAddr = (indexLastDeleted * 4)+1;
			eepromWriteBuffer(eepromAddr, (unsigned char*)(&srNext), 4);	
			//��������� ����� ������ ������
			srRecordBuffer = srNext;
			indexLastAdded = indexLastDeleted;
			IndexesUpdate(indexLastDeleted);//�������� ���������� ������� ��� ��������������
			disable_event_on_current =1;
			record_position =RecordsNumber;
			return 0x00;
	}
	else 
	{/*���������� ���������� - ������ �������� �� 100%*/};
return 0;
}

unsigned char DeleteCurrentRecord(void)
{
	EraseRecord(indexCurrentRecord);	//������� ������ ������
	if (IndexesUpdate(indexCurrentRecord)==0xFF)
	{
		//���������� ����
		AddRecord();//������������� �������� ���� ����� ������ ��� �������
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
//������� �������� ������� //�������������� � ������� �����������
srTime.dow_hours = date.dow <<5; 
srTime.dow_hours +=date.hours;
srTime.minutes = date.minutes;

for (indexNext=0;indexNext<99;indexNext++)
	{
	//������ ���������� NEXT
	if (disable_event_on_current && (indexNext==indexCurrentRecord)) continue;
	eeprAddr = (indexNext * 4)+1;
	eepromReadBuffer(eeprAddr, (unsigned char*)(&srNext), 4);
		if (srNext.attrib_a_pt & 0x80)
		{//������ �� ������	
			if (EventsCompare (&srNext,&srTime)== 0x00)
			{//������� �������� �������
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
//���������� �������� 
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
	//������ ������ ���������
	eeprAddr = (indexNext * 4)+1;
	//�������� ������ ���������
	eepromReadBuffer(eeprAddr, (unsigned char*)(&srSource), 4);
	if ((srSource.attrib_a_pt & 0x80) != 0) 
	{
	break;
	}

	//���� ������ ������ - ����� ���������� ������������� �������
		if (indexNext>indexSource)
		{//���������� ����� �����
			if (indexNext>98)
			{//�������� ����� - ������ ������ �� �������
				//��� ������� � �����������
				indexLastDeleted =98;//���������� ������ ���� �� 4 ����� �� ������
				RecordsNumber =0;
				return 0xFF;		
			}
			else
			{//���������� ����� �����
			 indexNext++;
			};
		}
		else
		{//������� ����� ����
			if (indexNext==0) 
			{//�������� ����� ����, ���������� � ������ �����
				indexNext=indexSource +1;}
			else 
			{//���������� ����� ����
				indexNext--;
			};
		};
	}while  (1);
//----------------------------------------------------------------------
	
disable_event_on_current =0;
//if (indexCurrentRecord != indexSource)
if (indexLastDeleted != indexSource)
{//������ �� ������� � ������ ������
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
	//���������� �������� ������ 1 ����� �������������
	for (indexNext=0;indexNext<99;indexNext++)
	{
	//������ ���������� NEXT
	eeprAddr = (indexNext * 4)+1;
	eepromReadBuffer(eeprAddr, (unsigned char*)(&srNext), 4);

		if (srNext.attrib_a_pt & 0x80)
		{//������ �� ������	
			RecordsNumber++; //������� �� ������� ������
			
			switch (EventsCompare (&srNext,&srMinimorum))
			{//��������� NEXT � MINIMORUM
				
				case 0x01:{//NEXT < MINIMORUM
					srMinimorum = srNext;
					indexMinimorum = indexNext;
				}break;

				default:{
					switch (EventsCompare (&srNext,&srMaximorum))
					{//��������� NEXT � MAXIMORUM
						case 0xFF:{//NEXT>MINIMORUM
							srMaximorum = srNext;
							indexMaximorum = indexNext;



						}break;

					};

				}break;
			};//switch
		}
		else
		{//������ ������
			indexLastDeleted = indexNext;
		};//if
	};//for

srStored_L = srMinimorum;
indexStored_L =indexMinimorum;
srStored_R = srMaximorum;
indexStored_R =indexMaximorum;	

	//���������� �������� ������ 2 ����� ����������� ������������
	for (indexNext=0;indexNext<99;indexNext++)
	{
	//������ ���������� NEXT
	eeprAddr = (indexNext * 4)+1;
	eepromReadBuffer(eeprAddr, (unsigned char*)(&srNext), 4);
		
		//���������� ��������
		if (srNext.attrib_a_pt & 0x80)
		{//������ �� ������		
			switch (EventsCompare (&srNext,&srSource))
			{//��������� NEXT � SOURCE
				case 0x01:{//NEXT < SOURCE

					record_position++;					

					switch (EventsCompare (&srNext,&srStored_L))
					{//��������� NEXT � STORED_L
						case 0xFF:{//NEXT>STORED_L
							srStored_L = srNext;
							indexStored_L = indexNext;
						}break;
					};//switch
				

				}break;

				case 0xFF:{//NEXT > SOURCE
					switch (EventsCompare (&srNext,&srStored_R))
					{//��������� NEXT � STORED_R
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
//change_mode 0-��������� 1-���������
	switch (index)
	{
		//--
		case 1:{//��������� DOW (.dow_hours)
			u_temp = srRecordBuffer.dow_hours & 0xE0;
			if (change_mode)
			{//���������
				if (u_temp == 0xC0) {u_temp=0;}else {u_temp +=0x20;};
			}
			else
			{//���������
				if (u_temp == 0x00) {u_temp=0xC0;}else{u_temp -=0x20;};
			};
			srRecordBuffer.dow_hours &=(~0xE0); 
			srRecordBuffer.dow_hours |= u_temp;	
		}break;
		//--
		case 2:{//��������� HOURS (.dow_hours)

			u_temp = srRecordBuffer.dow_hours & 0x1F; 
			if (change_mode)
			{//���������
				if (u_temp < 23) {u_temp++;}else {u_temp=0;};
			}
			else
			{//���������
				if (u_temp > 0) {u_temp--;}else {u_temp=23;};
			};
			srRecordBuffer.dow_hours &=~(0x1F);
			srRecordBuffer.dow_hours |=u_temp;
		}break;
		//--
		case 3:{//��������� MINUTES (.minutes)
			u_temp = srRecordBuffer.minutes; 
			if (change_mode)
			{//���������
				if (u_temp < 59) {u_temp++;}else {u_temp=0;};	
			}
			else
			{//���������
				if (u_temp > 0) {u_temp--;}else {u_temp=59;};
			}; 
			srRecordBuffer.minutes = u_temp;
		}break;
		//--
		case 4:{//��������� ����������� ������� 
			s_temp = srRecordBuffer.temperature;
			if (change_mode)
			{//���������	
				if (s_temp < 120) {s_temp++;}	 //60�������� ������� ��� ����  ������� ��� 0.5 ��������
			}
			else
			{//���������
				if (s_temp > -120) {s_temp--;};
			};
			srRecordBuffer.temperature =s_temp;
		}break;
		//--
		case 5:{//���������� ��������� ����� ������ ������� (alarm)
			srRecordBuffer.attrib_a_pt ^=0x04;
		}break;
		//--
		case 6:{//���������� ��������� ������ ������ ���� (���� ��������� / ���� �����������) P T
				//00-���� //01- ����� ���� ��� //10-����� ���� ���� //11-������������ �����������  
			u_temp = srRecordBuffer.attrib_a_pt & 0x03; 
			if (u_temp < 3) {u_temp++;}else {u_temp=0;};
			srRecordBuffer.attrib_a_pt &=(~0x03); 
			srRecordBuffer.attrib_a_pt |= u_temp;
			if (u_temp == 0x03) {srRecordBuffer.temperature = 0x25;/*18.5*/}
			else {srRecordBuffer.temperature = 0;};
		}break;
	};//switch
		//���������� ������
		SaveRecord (indexCurrentRecord);
		LoadRecord (indexCurrentRecord);
}



/////////////////////////////////////////////////////////////////////////////
//������������� ������������ ���������
void UpdateBissextile(void)
{//�������� ������� ������������ ���� (bissextile)
//60 ����
	date.bissextile = 0;
	if ((date.years%4 == 0) && ((date.years % 100 != 0) || (date.years % 400 == 0))){date.bissextile =1;};
}

void UpdateMonthDays(void)
{
	date.mdays= pgm_read_byte(&mdays[date.months-1]);	
	if (date.months ==2) {date.mdays = date.mdays + date.bissextile;};
}


void UpdateDOW(void)
{//�������� ���� ������ 0:�� 1:�� 2:�� 3:�� 4:�� 5:�� 6:c�
//184 ����
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
	{//���������� ������
		date.seconds++;
		if (date.seconds > 59)
		{
			date.seconds =0;
				{//���������� �����
					date.minutes++;
					if (date.minutes>59)
					{
						date.minutes=0;
							{//���������� �����
								date.hours++;
								if (date.hours>23)
								{
									date.hours=0;
										{//���������� ���			
												date.dow++;
												if (date.dow>6)
												{//������������� ��� ������
													date.dow=0;
												};
											date.days++;
											if (date.mdays<date.days)
											{
												date.days =1;
					{//���������� ������---------------------------------			
						date.months++;
						if (date.months > 12)
						{
							date.months =1;
							{//���������� ����
								date.years++;		
								UpdateBissextile();//��� �������,���������� bissextile ����� ��������� ����		
							}//���������� ����	
						};
						UpdateMonthDays();
					}//���������� ������--------------------------------------
											}			 		
										}//���������� ���
								};

							}//���������� �����

					};
				}//���������� �����
		};
	}//���������� ������
return;
}





void ChangeDATE (unsigned char change_mode,unsigned char index);
void ChangeDATE (unsigned char change_mode,unsigned char index)
{
	switch(index)
	{
	case 0:{//����� ������
		date.seconds =0;
	}break;
	//--
	case 1:{//��������� �����
		if (change_mode)
		{//���������
			if (date.minutes < 59) {date.minutes++;}else {date.minutes =0;}
		}
		else
		{//���������
			if (date.minutes ==0) {date.minutes =59;}else{date.minutes--;};
		};
	}break;
	//--
	case 2:{//��������� �����
		if (change_mode)
		{//���������	
			if (date.hours < 23){date.hours ++;}else{date.hours =0;};
		}
		else
		{//���������
			if (date.hours ==0){date.hours =23;}else{date.hours --;};
		};
	}break;
	//--
	case 3:{//��������� ���� � ��� ������
		if (change_mode)
		{//���������
			if (date.days <date.mdays){date.days++;}else{ date.days=1;};
			if(date.dow < 6){date.dow++;}else{date.dow=0;};
		}
		else
		{//���������
			if (date.days ==1){date.days = date.mdays;}else{date.days--;};
			if(date.dow == 0){date.dow = 6;}else{date.dow--;};
		};
	}break;
	//--
	case 4:{//��������� ������
		if (change_mode)
		{//���������	
			if(date.months<12){date.months++;}else{date.months=1;};
		}
		else
		{//���������
			if(date.months==1){date.months=12;}else{date.months--;};
		};
		UpdateMonthDays();
		if (date.days>date.mdays) date.days = date.mdays;
		UpdateDOW();
	}break;
	//--
	case 5:{//��������� ����
		if (change_mode)
		{//���������
			date.years++;
		}
		else
		{//���������
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





