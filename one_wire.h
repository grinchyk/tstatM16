/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// OneWire Instructions
	#define     OW_READ_ROM    				0x33    //READ ROM command code.
	#define     OW_MATCH_ROM   				0x55    //MATCH ROM command code.
	#define     OW_SKIP_ROM	    			0xCC    //SKIP ROM command code.
	#define     OW_SEARCH_ROM  				0xF0    //SEARCH ROM command code.
	#define     OW_ALARM_SEARCH		     	0xEC    //Search finished return code.
	#define     OW_WRIE_SCRATCHPAD			0x4E
	#define 	OW_READ_SCRATCHPAD 			0xBE	//???
	#define 	OW_COPY_SCRATCHPAD			0x48
	#define 	OW_CONVERT_T				0x44
	#define 	OW_RECALL_E2                0xB8
	#define 	OW_READ_POWER_SUPPLY		0xB4
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	#define     OW_ERROR_CRC				-80
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	#define     CPU_F   8
	#define     LAG_PERIOD    3   //запаздывание задержки
//OneWire Bus Timing выражено в Nclk //Nclk = N(мкс)*CPU_F(MHz) //8Mгц: [1мкс<->8clk]
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//стандартные временные задержки OneWire 
	#define     OW_DELAY_T_SLOT		120	* CPU_F		//Time Slot					 60-120
	#define     OW_DELAY_T_REC		  1	* CPU_F		//Recovery Time				  1-...
	#define     OW_DELAY_T_LOW0		100	* CPU_F		//Write 0 Low Time			 60-120
	#define     OW_DELAY_T_LOW1		 5	* CPU_F		//Write 1 Low Time			  1-15
	#define     OW_DELAY_T_RDV		 13	* CPU_F		//Read Data Valid			...-15
	#define     OW_DELAY_T_RDV_1	 6	* CPU_F		//Read Data Valid			...-15
	#define     OW_DELAY_T_RDV_2	 7	* CPU_F		//Read Data Valid			...-15
	#define     OW_DELAY_T_RSTH		480	* CPU_F		//Reset Time High			480-...
	#define     OW_DELAY_T_RSTL		480	* CPU_F		//Reset Time Low			480-...
	#define     OW_DELAY_T_PDHIGH	 45	* CPU_F		//Presence Detect High		 15-60
	#define     OW_DELAY_T_PDLOW	120	* CPU_F		//Presence Detect Low		 60-240
	#define     OW_DELAY_T_SPON		 10	* CPU_F		//Time to Strong Pull Up On	   -10
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//производные и вспомогательные определения задержек	
//#define     OW_DELAY_TD_0        %%%
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	#define     OW_PORT        PORTC   //данные порта  read  								//write port value
	#define     OW_PIN         PINC    //состояние битов порта read only					//0-input 1-output
	#define     OW_DDR         DDRC    //упраление направлением работы порта вход выход		//read only port value  
	#define		OW_MASK		   (1<<PC5)	   //маска шины
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Формат хранилища DS1820
//byte 0 Temperature LSB (AAh)\  (+85°C)         //температура младший бит - 0.5 градуса
//byte 1 Temperature MSB (00h)/                  //температура знак 0x00:"+" / 0xFF:"-"
//byte 2 TH Register or User Byte 1* TH 	       Register or User Byte 1
//byte 3 TL Register or User Byte 2* TL 	       Register or User Byte 2
//byte 4 Reserved (FFh)
//byte 5 Reserved (FFh)
//byte 6 COUNT REMAIN (0Ch)
//byte 7 COUNT PER °C (10h)
//byte 8 CRC*
//unsigned char PreviousPanel=0;
