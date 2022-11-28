#define	PANELS		0x05
#define E_RECORDS 100 //396 ����� EEPROM 0-99
///////////////////////////////////////////
//DRAW MODES
#define POS						0x00 //const positive (default)
#define NEG						0x01 //const negative
#define FLO						0x02 //flash on/off
#define FLI						0x03 //flash inverse
///////////////////////////////////////////

///////////////////////////////////////////
//FIELD TYPES
#define F_EPN						0x00
#define F_YEA						0x10	
#define F_MON	 					0x20	
#define F_DAY 						0x30	
#define F_DOW 						0x40	
#define F_HOR 						0x50
#define F_MIN    					0x60	
#define F_SEC    				 	0x70
#define F_TPS						0x80
#define F_PST	 					0x90
#define F_PMM						0xA0
#define F_PRI						0xB0
#define F_PRP						0xB1
#define F_RCA	 					0xC0
#define F_RCT	 					0xD0
#define F_TXT						0xE0
#define F_CMD						0xF0

//��� ��������
//http://www.nongnu.org/avr-libc/user-manual/group__avr__pgmspace.html

/////////////////////////////////////////////////////////////////////
//���� ������� ����

const unsigned char T00[] PROGMEM ={"����:"};
const unsigned char T01[] PROGMEM ={"���"};
const unsigned char T02[] PROGMEM ={"���"};
const unsigned char T03[] PROGMEM ={"���"};
const unsigned char T04[] PROGMEM ={"���"};
const unsigned char T05[] PROGMEM ={"����:"};
const unsigned char T06[] PROGMEM ={"���������"};
const unsigned char T07[] PROGMEM ={"����/�����"};
const unsigned char T08[] PROGMEM ={"�����"};
const unsigned char T09[] PROGMEM ={"���"};
const unsigned char T0A[] PROGMEM ={"���"};
const unsigned char T0B[] PROGMEM ={"�������"};
const unsigned char T0C[] PROGMEM ={"/"};
const unsigned char T0D[] PROGMEM ={"."};
const unsigned char T0E[] PROGMEM ={":"};
const unsigned char T0F[] PROGMEM ={"+"};
const unsigned char T10[] PROGMEM ={"��"};
const unsigned char T11[] PROGMEM ={"1:"};
const unsigned char T12[] PROGMEM ={"2:"};
const unsigned char T13[] PROGMEM ={"\0"};

const unsigned char *tokens [] PROGMEM=
{
	T00,T01,T02,T03,
	T04,T05,T06,T07,
	T08,T09,T0A,T0B,
	T0C,T0D,T0E,T0F,
	T10,T11,T12,T13
};


#define OP(op)	(F_CMD+op)
#define AT(x,y) (x+16*y)//������ ��������� 	**YYXXXX
#define DM(c_dm,f_dm)	(c_dm * 16 + f_dm)
#define TOKEN(t) (t)
#define	SUB(f) (f)
#define DAT(d) (d)
#define PAN(p) (p)


	//OP(11) ���������� ������ �� ��������� ��� ��������
	//OP(10) ���������� ������ ��������� � ������� ��������
	//OP(8) ������� ��� �����������
	//OP(6) ������� ������ � ��������

	//SUB(10)
	//SUB()

const unsigned char Panels [][5] PROGMEM =
{
///////////////////////////////////////////
//{_2007_���_05_}
//{_��_19:52:52_}
//{+22.5/+22.5^C}
//{����:���___^_}
///////////////////////////////////////////
//
//������ 0	//{_2007_���_05_}
	{OP(11),0x00, 0x00, SUB(0), PAN(0)},// DEFAULT
	{OP(10),0x00, 0x00, SUB(0), PAN(4)},//������ �������� �� ������ ��
	{F_YEA,	AT(1,0), DM(POS,POS), SUB(0), DAT(0)},//
	{F_MON,	AT(6,0), DM(POS,POS), SUB(0), DAT(0)},//
	{F_DAY,	AT(10,0),DM(POS,POS), SUB(0), DAT(6)},//
//������ 1	//{_��_19:52:52_}
	{F_DOW,	AT(1,1),DM(POS,POS),SUB(0),DAT(0)},//
	{F_HOR,	AT(4,1),DM(POS,POS),SUB(2),DAT(2)},//
	{F_TXT,	AT(6,1),DM(POS,FLO),SUB(0),TOKEN(0x0E)},//
	{F_MIN,	AT(7,1),DM(POS,POS),SUB(0),DAT(0)},//
	{F_TXT,	AT(9,1),DM(POS,FLO),SUB(0),TOKEN(0x0E)},//
	{F_SEC,	AT(10,1),DM(POS,POS),SUB(0),DAT(0)},//
//������ 2	//{+22.5/+22.5^C}
	{F_TPS,	AT(0,2),DM(POS,POS),SUB(0),DAT(1)},//
	{F_TXT,	AT(5,2),DM(POS,POS),SUB(0),TOKEN(0x0C)},///
	{F_TXT,	AT(11,2),DM(POS,POS),SUB(0),TOKEN(0x10)},//��
//������ 3	//{_����:_���_^_}
	{F_TXT,	AT(1,3),DM(POS,POS),SUB(0),TOKEN(0x00)},//����
	{F_TXT,	AT(13,13),DM(NEG,NEG),SUB(10),TOKEN(0x13)},//��������� ������
	{F_TPS,	AT(6,2),DM(NEG,POS),SUB(7),DAT(5)},//�������
	{F_TXT,	AT(13,13),DM(NEG,NEG),SUB(10),TOKEN(0x13)},//��������� ������												
	
	{F_PMM,	AT(7,3),DM(NEG,POS),SUB(12),DAT(0)},//������� ����� ����//�������� ������ ��������� ����												
	
	{F_PST,	AT(11,3),DM(POS,POS),SUB(0),DAT(0)},//��������� ��������� ����

    
{F_EPN,0x00,0x00,SUB(0),0x00},//����� ������

///////////////////////////////////////////
//{_����/�����__}
//{_2007_���_05_}
//{_��_00:00:00_}
//{____�����____}
///////////////////////////////////////////
// 
//������ 0	//{_����/�����__}
	{OP(11),	0x00, 0x00, SUB(0), PAN(0)},//	
	{F_TXT,	AT(1, 0),DM(POS,POS),SUB(0),TOKEN(0x07)},//


//������ 1	//{_2007_���_05_}
	{F_YEA,	AT(1,1),DM(FLI,POS),SUB(2),DAT(5)},//
	{F_MON,	AT(6,1),DM(FLI,POS),SUB(2),DAT(4)},//
	{F_DAY,	AT(10,1),DM(FLI,POS),SUB(2),DAT(3)},//


//������ 2	//{_��_00:00:00_}
	{F_DOW,	AT(1,2),DM(POS,POS),SUB(0),DAT(0)},//
	{F_HOR,	AT(4,2),DM(FLI,POS),SUB(2),DAT(2)},//
	{F_TXT,	AT(6,2),DM(POS,POS),SUB(0),TOKEN(0x0E)},//
	{F_MIN,	AT(7,2),DM(FLI,POS),SUB(2),DAT(1)},//
	{F_TXT,	AT(9,2),DM(POS,POS),SUB(0),TOKEN(0x0E)},//
	{F_SEC,	AT(10,2),DM(FLI,POS),SUB(2),DAT(0)},//

//������ 3	//{____�����____}
	{OP(10),	0x00, 0x00, SUB(0), PAN(3)},//������ �������� �� ������ ��
	{F_TXT,	AT(4,3),DM(NEG,POS),SUB(10),TOKEN(0x08)},//
//����� ������
{F_EPN,0x00,0x00,SUB(0),0x00},//����� ������
///////////////////////////////////////////
//{������� 00/00}
//{��_00:00___C_}
//{+22.5��____T_}
//{���_���_�����}
///////////////////////////////////////////
//
//������ 0  //{������� 00/00}
	{OP(11),	0x00, 0x00, SUB(0), PAN(0)},// ������ �������� �� ���������	

	{F_TXT,	AT(0,0),DM(POS,POS),SUB(0),TOKEN(0x0B)},//����� ������� �������													
	{F_PRP,	AT(8,0),DM(NEG,POS),SUB(4),DAT(0)},    // 
	{F_TXT,	AT(10,0),DM(POS,POS),SUB(0),TOKEN(0x0C)},//																									
	{F_PRI,	AT(11,0),DM(POS,POS),SUB(4),DAT(0)},    // 
//������ 1	//{��_00:00___C_}
	{F_DOW ,AT(0,1),DM(FLI,POS),SUB(3),DAT(1)},//
	{F_HOR ,AT(3,1),DM(FLI,POS),SUB(3),DAT(2)},//

	{F_TXT,	AT(5,1),DM(POS,POS),SUB(0),TOKEN(0x0E)},//
	{F_MIN ,AT(6,1),DM(FLI,POS),SUB(3),DAT(3)},//
	{F_RCA ,AT(11,1),DM(FLI,POS),SUB(3),DAT(5)},//
//������ 2	//{+22.5��____T_}			
	{F_RCT ,AT(11,2),DM(FLI,POS),SUB(3),DAT(6)},//
	{F_TPS ,AT(0,2),DM(FLI,POS),SUB(3),DAT(4)},//
	{F_TXT,	AT(5,2),DM(POS,POS),SUB(0),TOKEN(0x10)},//��
//������ 3	//{���_���_�����}
	{F_TXT,	AT(0,3),DM(NEG,POS),SUB(6),TOKEN(0x0A)},//
	{F_TXT,	AT(4,3),DM(NEG,POS),SUB(5),TOKEN(0x09)},//
	{OP(10),0x00,0x00,SUB(0),PAN(3)},//	������ �������� �� OP(10) ����
	{F_TXT,	AT(8,3),DM(NEG,POS),SUB(10),TOKEN(0x08)},//
//����� ������
{F_EPN,0x00,0x00,SUB(0),0x00},//����� ������
///////////////////////////////////////////
//{____����_____}
//{1:����/�����_}//
//{2:���������__}//F_PAN
//{____�����____}
///////////////////////////////////////////
//
	{OP(11),	0x00, 0x00, SUB(0), PAN(0)},//������ �������� �� ���������
//������ 0	//{____����_____}
	{F_TXT,	AT(4,0),DM(POS,POS),SUB(0),TOKEN(0x05)},//
//������ 1	//{1:����/�����_}
	{F_TXT,	AT(0,1),DM(POS,POS),SUB(0),TOKEN(0x11)},//
	{OP(10),0x00,0x00,SUB(0),PAN(1)},//	������ �������� �� SUB(10) ����
	{F_TXT,	AT(2,1),DM(NEG,POS),SUB(10),TOKEN(0x07)},//

//������ 2	//{2:���������__}
	{F_TXT,	AT(0,2),DM(POS,POS),SUB(0),TOKEN(0x12)},//
	{OP(10),0x00,0x00,SUB(0),PAN(2)},//	������ �������� �� SUB(10) ����
	{F_TXT,	AT(2,2),DM(NEG,POS),SUB(10),TOKEN(0x06)},//

//������ 3	//{____�����____}
	{OP(10),0x00,0x00,SUB(0),PAN(0)},//	������ �������� �� SUB(10) ����
	{F_TXT,	AT(4,3),DM(NEG,POS),SUB(10),TOKEN(0x08)},//
//����� ������	
{F_EPN,0x00,0x00,SUB(0),0x00},//����� ������


///////////////////////////////////////////
//{____����_____}
//{*************}//F_PAN
//{*************}//F_PAN
//{____�����____}
///////////////////////////////////////////


///////////////////////////////////////////
//{����/����� }
//{���������  }
//{���������  }
//{���������  }
///////////////////////////////////////////





///////////////////////////////////////////
//{19:52_^_+22��}
//{             }
//{             }
//{             }
///////////////////////////////////////////
//
	{OP(11),	0x00, 0x00, SUB(0), PAN(0)},// ������ �������� �� ���������
	{OP(10),	0x00, 0x00, SUB(0), PAN(3)},// ������ �������� �� ������ ��
//������ 1	//{19:52_^_+22��}
	{F_HOR,	AT(0,0),DM(POS,POS),SUB(2),DAT(2)},//
	{F_TXT,	AT(2,0),DM(POS,FLO),SUB(0),TOKEN(0x0E)},//
	{F_MIN,	AT(3,0),DM(POS,POS),SUB(0),DAT(0)},//
	{F_PST,	AT(6,0),DM(POS,POS),SUB(0),DAT(0)},//
	{F_TPS,	AT(8,0),DM(POS,POS),SUB(0),DAT(0)},//
	{F_TXT,	AT(11,0),DM(POS,POS),SUB(0),TOKEN(0x10)},//��	
	{OP(12), 0x00, 0x00, SUB(0), PAN(0)},//��������� ������� �����������
{F_EPN,0x00,0x00,SUB(0),0x00},//����� ������


{F_EPN,0xFF,0x00,SUB(0),0x00}//����� ����� �������
///////////////////////////////////////////
};

const unsigned char fnt_PG [4][5] PROGMEM=
{
{0x00,0x04,0x0A,0x0A,0x04},//������ �������     0xB0
{0x00,0x04,0x0A,0x0A,0x04},//������ �������		0xB1
{0x08,0x04,0x7E,0x04,0x08},//������� �����   	0xB2
{0x10,0x20,0x7E,0x20,0x10} //������� ����   	0xB3
};



const unsigned char fnt_EN [32][5] PROGMEM=
{
{0x00,0x00,0x00,0x00,0x00},
{0x00,0x00,0x5F,0x00,0x00},
{0x00,0x03,0x00,0x03,0x00},
{0x14,0x7F,0x14,0x7F,0x14},
{0x26,0x49,0x7F,0x49,0x32},
{0x26,0x16,0x08,0x34,0x32},
{0x40,0x40,0x40,0x40,0x40},
{0x00,0x00,0x07,0x00,0x00},
{0x1C,0x22,0x41,0x00,0x00},
{0x00,0x00,0x41,0x22,0x1C},
{0x2A,0x1C,0x3E,0x1C,0x2A},
{0x08,0x08,0x3E,0x08,0x08},
{0x00,0x00,0x60,0xE0,0x00},
{0x10,0x10,0x10,0x10,0x10},
{0x00,0x00,0x60,0x60,0x00},
{0x20,0x10,0x08,0x04,0x02},
{0x3E,0x51,0x49,0x45,0x3E},
{0x00,0x42,0x7F,0x40,0x00},
{0x42,0x61,0x51,0x49,0x46},
{0x22,0x41,0x49,0x49,0x36},
{0x30,0x28,0x24,0x22,0x7F},
{0x27,0x45,0x45,0x45,0x39},
{0x3E,0x49,0x49,0x49,0x32},
{0x01,0x61,0x11,0x09,0x07},
{0x36,0x49,0x49,0x49,0x36},
{0x26,0x49,0x49,0x49,0x3E},
{0x00,0x00,0x36,0x00,0x00},
{0x00,0x40,0x36,0x00,0x00},
{0x00,0x18,0x24,0x42,0x00},
{0x14,0x14,0x14,0x14,0x14},
{0x00,0x42,0x24,0x18,0x00},
{0x02,0x01,0x51,0x09,0x06}
};

const unsigned char fnt_RU [32][5] PROGMEM=
{
{0x78,0x14,0x12,0x12,0x7E},
{0x7E,0x4A,0x4A,0x4A,0x30},
{0x7E,0x4A,0x4A,0x4A,0x34},
{0x7E,0x02,0x02,0x02,0x02},
{0xC0,0x7C,0x42,0x42,0xFE},
{0x7E,0x4A,0x4A,0x4A,0x42},
{0x76,0x08,0x7E,0x08,0x76},
{0x42,0x4A,0x4A,0x4A,0x34},
{0x7E,0x10,0x08,0x04,0x7E},
{0x7E,0x10,0x09,0x04,0x7E},
{0x7E,0x08,0x08,0x14,0x62},
{0x78,0x04,0x02,0x02,0x7E},
{0x7E,0x04,0x08,0x04,0x7E},
{0x7E,0x08,0x08,0x08,0x7E},
{0x3C,0x42,0x42,0x42,0x3C},
{0x7E,0x02,0x02,0x02,0x7E},
{0x7E,0x12,0x12,0x12,0x0C},
{0x3C,0x42,0x42,0x42,0x42},
{0x02,0x02,0x7E,0x02,0x02},
{0x4E,0x50,0x50,0x50,0x3E},
{0x1C,0x22,0x7E,0x22,0x1C},
{0x62,0x14,0x08,0x14,0x62},
{0x7E,0x40,0x40,0x40,0xFE},
{0x0E,0x10,0x10,0x10,0x7E},
{0x7E,0x40,0x7E,0x40,0x7E},
{0x7E,0x40,0x7E,0x40,0xFE},
{0x02,0x7E,0x48,0x48,0x30},
{0x7E,0x48,0x30,0x00,0x7E},
{0x7E,0x48,0x48,0x48,0x30},
{0x42,0x4A,0x4A,0x4A,0x3C},
{0x7E,0x08,0x3C,0x42,0x3C},
{0x4C,0x32,0x12,0x12,0x7E}
};

