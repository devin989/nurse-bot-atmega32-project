#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <inttypes.h>

#include "lcd_hd44780_avr.h"

#include "custom_char.h"

#define _CONCAT(a, b) a##b
#define PORT(x) _CONCAT(PORT, x)
#define PIN(x) _CONCAT(PIN, x)
#define DDR(x) _CONCAT(DDR, x)

#define LCD_DATA_PORT PORT(LCD_DATA)
#define LCD_E_PORT PORT(LCD_E)
#define LCD_RS_PORT PORT(LCD_RS)
#define LCD_RW_PORT PORT(LCD_RW)

#define LCD_DATA_DDR DDR(LCD_DATA)
#define LCD_E_DDR DDR(LCD_E)
#define LCD_RS_DDR DDR(LCD_RS)
#define LCD_RW_DDR DDR(LCD_RW)

#define LCD_DATA_PIN PIN(LCD_DATA)

#define SET_E() (LCD_E_PORT |= (1 << LCD_E_POS))
#define SET_RS() (LCD_RS_PORT |= (1 << LCD_RS_POS))
#define SET_RW() (LCD_RW_PORT |= (1 << LCD_RW_POS))

#define CLEAR_E() (LCD_E_PORT &= (~(1 << LCD_E_POS)))
#define CLEAR_RS() (LCD_RS_PORT &= (~(1 << LCD_RS_POS)))
#define CLEAR_RW() (LCD_RW_PORT &= (~(1 << LCD_RW_POS)))

#ifdef LCD_TYPE_162
#define LCD_TYPE_204
#endif

#ifdef LCD_TYPE_202
#define LCD_TYPE_204
#endif

void LCDByte(uint8_t c, uint8_t isdata)
{
	//Sends a byte to the LCD in 4bit mode
	//isdata=1 for data
	//isdata=0 for command

	//NOTE: THIS FUNCTION RETURS ONLY WHEN LCD HAS COMPLETED PROCESSING THE COMMAND

	uint8_t hn, ln; //Nibbles
	uint8_t temp;

	hn = c >> 4;
	ln = (c & 0x0F);

	if (isdata == 0)
		CLEAR_RS();
	else
		SET_RS();

	_delay_us(0.500); //tAS

	SET_E();

	//Send high nibble

	temp = (LCD_DATA_PORT & (~(0X0F << LCD_DATA_POS))) | ((hn << LCD_DATA_POS));
	LCD_DATA_PORT = temp;

	_delay_us(1); //tEH

	//Now data lines are stable pull E low for transmission

	CLEAR_E();

	_delay_us(1);

	//Send the lower nibble
	SET_E();

	temp = (LCD_DATA_PORT & (~(0X0F << LCD_DATA_POS))) | ((ln << LCD_DATA_POS));

	LCD_DATA_PORT = temp;

	_delay_us(1); //tEH

	//SEND

	CLEAR_E();

	_delay_us(1); //tEL

	LCDBusyLoop();
}

void LCDBusyLoop()
{
	//This function waits till lcd is BUSY

	uint8_t busy, status = 0x00, temp;

	//Change Port to input type because we are reading data
	LCD_DATA_DDR &= (~(0x0f << LCD_DATA_POS));

	//change LCD mode
	SET_RW();   //Read mode
	CLEAR_RS(); //Read status

	//Let the RW/RS lines stabilize

	_delay_us(0.5); //tAS

	do
	{

		SET_E();

		//Wait tDA for data to become available
		_delay_us(0.5);

		status = (LCD_DATA_PIN >> LCD_DATA_POS);
		status = status << 4;

		_delay_us(0.5);

		//Pull E low
		CLEAR_E();
		_delay_us(1); //tEL

		SET_E();
		_delay_us(0.5);

		temp = (LCD_DATA_PIN >> LCD_DATA_POS);
		temp &= 0x0F;

		status = status | temp;

		busy = status & 0b10000000;

		_delay_us(0.5);
		CLEAR_E();
		_delay_us(1); //tEL
	} while (busy);

	CLEAR_RW(); //write mode
	//Change Port to output
	LCD_DATA_DDR |= (0x0F << LCD_DATA_POS);
}

void LCDInit(uint8_t style)
{
	/*****************************************************************
	
	This function Initializes the lcd module
	must be called before calling lcd related functions

	Arguments:
	style = LS_BLINK,LS_ULINE(can be "OR"ed for combination)
	LS_BLINK :The cursor is blinking type
	LS_ULINE :Cursor is "underline" type else "block" type

	*****************************************************************/

	//After power on Wait for LCD to Initialize
	_delay_ms(100);

	//Clear Ports
	LCD_DATA_PORT &= (~(0x0F << LCD_DATA_POS));

	CLEAR_E();
	CLEAR_RW();
	CLEAR_RS();

	//Set IO Ports direction
	LCD_DATA_DDR |= (0x0F << LCD_DATA_POS); //data line direction
	LCD_E_DDR |= (1 << LCD_E_POS);			//E line line direction
	LCD_RS_DDR |= (1 << LCD_RS_POS);		//RS line direction
	LCD_RW_DDR |= (1 << LCD_RW_POS);		//RW line direction

	//Reset sequence
	/*
	_delay_us(0.3);	//tAS
	
	SET_E();
	LCD_DATA_PORT|=((0b00000011)<<LCD_DATA_POS);
	_delay_us(1);
	CLEAR_E();
	_delay_us(1);
	
	_delay_ms(10);
	
	SET_E();
	LCD_DATA_PORT|=((0b00000011)<<LCD_DATA_POS);
	_delay_us(1);
	CLEAR_E();
	_delay_us(1);
	
	_delay_us(200);
	
	SET_E();
	LCD_DATA_PORT|=((0b00000011)<<LCD_DATA_POS);
	_delay_us(1);
	CLEAR_E();
	_delay_us(1);
	
	_delay_us(200);
	*/
	//Reset sequence END

	//Set 4-bit mode
	_delay_us(0.3); //tAS

	SET_E();
	LCD_DATA_PORT |= ((0b00000010) << LCD_DATA_POS); //[B] To transfer 0b00100000 i was using LCD_DATA_PORT|=0b00100000
	_delay_us(1);
	CLEAR_E();
	_delay_us(1);

	//Wait for LCD to execute the Functionset Command
	//LCDBusyLoop();                                    //[B] Forgot this delay
	_delay_us(300);

	//Now the LCD is in 4-bit mode

	LCDCmd(0b00101000);			//function set 4-bit,2 line 5x7 dot format
	LCDCmd(0b00001000 | style); //Display Off
	LCDCmd(0b00001100 | style); //Display On

	/* Custom Char */
	LCDCmd(0b01000000);

	uint8_t __i;
	for (__i = 0; __i < sizeof(__cgram); __i++)
		LCDData(__cgram[__i]);

	LCDClear();
}
void LCDWriteString(const char *msg)
{
	/*****************************************************************
	
	This function Writes a given string to lcd at the current cursor
	location.

	Arguments:
	msg: a null terminated string to print

	Their are 8 custom char in the LCD they can be defined using
	"LCD Custom Character Builder" PC Software. 

	You can print custom character using the % symbol. For example
	to print custom char number 0 (which is a degree symbol), you 
	need to write

		LCDWriteString("Temp is 30%0C");
		                          ^^
								  |----> %0 will be replaced by
								  		custom char 0.

	So it will be printed like.
		
		Temp is 30�C
		
	In the same way you can insert any syblom numbered 0-7 	


	*****************************************************************/
	while (*msg != '\0')
	{
		//Custom Char Support
		if (*msg == '%')
		{
			msg++;
			int8_t cc = *msg - '0';

			if (cc >= 0 && cc <= 7)
			{
				LCDData(cc);
			}
			else
			{
				LCDData('%');
				LCDData(*msg);
			}
		}
		else
		{
			LCDData(*msg);
		}
		msg++;
	}
}

void LCDWriteFString(const char *msg)
{
	/*****************************************************************
	
	This function Writes a given string to lcd at the current cursor
	location.

	Arguments:
	msg: a null terminated string to print

	Their are 8 custom char in the LCD they can be defined using
	"LCD Custom Character Builder" PC Software. 

	You can print custom character using the % symbol. For example
	to print custom char number 0 (which is a degree symbol), you 
	need to write

		LCDWriteString("Temp is 30%0C");
		                          ^^
								  |----> %0 will be replaced by
								  		custom char 0.

	So it will be printed like.
		
		Temp is 30�C
		
	In the same way you can insert any syblom numbered 0-7 	


	*****************************************************************/

	char ch = pgm_read_byte(msg);
	while (ch != '\0')
	{
		//Custom Char Support
		if (ch == '%')
		{
			msg++;

			ch = pgm_read_byte(msg);

			int8_t cc = ch - '0';

			if (cc >= 0 && cc <= 7)
			{
				LCDData(cc);
			}
			else
			{
				LCDData('%');
				LCDData(ch);
			}
		}
		else
		{
			LCDData(ch);
		}
		msg++;
		ch = pgm_read_byte(msg);
	}
}

void LCDWriteInt(int val, int8_t field_length)
{
	/***************************************************************
	This function writes a integer type value to LCD module

	Arguments:
	1)int val	: Value to print

	2)unsigned int field_length :total length of field in which the value is printed
	must be between 1-5 if it is -1 the field length is no of digits in the val

	****************************************************************/

	char str[5] = {0, 0, 0, 0, 0};
	int i = 4, j = 0;

	//Handle negative integers
	if (val < 0)
	{
		LCDData('-');   //Write Negative sign
		val = val * -1; //convert to positive
	}
	else
	{
		LCDData(' ');
	}

	while (val)
	{
		str[i] = val % 10;
		val = val / 10;
		i--;
	}

	if (field_length == -1)
		while (str[j] == 0)
			j++;
	else
		j = 5 - field_length;

	for (i = j; i < 5; i++)
	{
		LCDData('0' + str[i]);
	}
}
void LCDGotoXY(uint8_t x, uint8_t y)
{
	if (x >= 20)
		return;

#ifdef LCD_TYPE_204

	switch (y)
	{
	case 0:
		break;
	case 1:
		x |= 0b01000000;
		break;
	case 2:
		x += 0x14;
		break;
	case 3:
		x += 0x54;
		break;
	}

#endif

#ifdef LCD_TYPE_164
	switch (y)
	{
	case 0:
		break;
	case 1:
		x |= 0b01000000;
		break;
	case 2:
		x += 0x10;
		break;
	case 3:
		x += 0x50;
		break;
	}

#endif

	x |= 0b10000000;
	LCDCmd(x);
}
void LCDWriteLongInt(int32_t val, int8_t field_length)
{
	/***************************************************************
	This function writes a integer type value to LCD module

	Arguments:
	1)int val	: Value to print

	2)unsigned int field_length :total length of field in which the value is printed
	must be between 1-5 if it is -1 the field length is no of digits in the val

	****************************************************************/

	char str[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int i = 9, j = 0;

	//Handle negative integers
	if (val < 0)
	{
		LCDData('-');   //Write Negative sign
		val = val * -1; //convert to positive
	}
	else
	{
		LCDData(' ');
	}

	while (val)
	{
		str[i] = val % 10;
		val = val / 10;
		i--;
	}

	if (field_length == -1)
		while (str[j] == 0)
			j++;
	else
		j = 10 - field_length;

	for (i = j; i < 10; i++)
	{
		LCDData('0' + str[i]);
	}
}