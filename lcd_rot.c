//-----------------------------------------------------------------------------
//lcd_rot.c
//-----------------------------------------------------------------------------
//
// Program Description:
//
// This program  interfaces with DS1307 ( Real time clock ) via the i2c communication protocal.
// It sets the time & calender registers and runs the clock.
//
// How To Test:
//
// 1) Download code to the Pt-51 board
// 2) Connect  LCD at the slot provided
// 3) You should see a scrolling display of the following text " This is a string message " from right to left only
//
//
// Author:				 P.Anusri
// Target:         C8051F38x
// Tool chain:     Keil / Raisonance
// Command Line:   None
// 
//
// Release 1.0 - 10-JUNE-2013 (PKC)
//    -Initial Revision
//

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include <at89c5131.h>

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

Sbit ( RW , P0 , 1 ) ;
Sbit ( RS , P0 , 0 ) ;
Sbit ( EN , P0 , 2 ) ;
Sfr ( Dt , 0xA0 ) ;    

unsigned char msg_size=24;
code unsigned char message[24]="This is a string message"; // write message to be displayed here

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

signed char p;
unsigned long i;
unsigned char temp;
unsigned char count;
signed char boundary; 

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void delay();
void status ();
void command ( unsigned char a );
void display (unsigned char a);
	
//-----------------------------------------------------------------------------
// main() Routine
//-----------------------------------------------------------------------------
			
void main()
{ 
	command(0x38); // function set command
	command(0x0c); // display switch command
	command(0x06); // input set command
	command(0x01); // clear screen command
	
	boundary=0; // new character in text message to be displayed
	
	/* boundary pointer decides the character to be displayed at the extreme right of the LCD screen and p is another pointer that runs
	from the boundary character back to the first charcter in the text message */
	
	
	while(1)
	{
		for(boundary=0;boundary<=(msg_size-1);boundary++) // incrementing the position of character  to be displayed in text
		{
			command(0x01); // clear screen command
			p=boundary;
			count=0; 
			while(p>=0 && count<=15) // display all characters that come before the boundary character and that can fit on to the screen
			{
				command(0x8f-count); // decides the postion of text on LCD screen ( from right of the LCD screen )
				display(message[p]); // display corresponding character from text at the position decided in the prev command
			
				p=p-1;
				count=count+1;
			}
			delay(); // controls the scrolling speed
		}
	
	/* P is pointer that decides the position of the last charcter in the text and boundary pointer runs from last 
	character to either the first character or to the charcter that last can fit on the screen */
		
		p=1; 
		while(p<=15)
		{
			command(0x01); // clear screen command
			boundary=msg_size-1;
			count=p; // decides the position,from the right of the screen,of the last character in text message
			while(boundary>=0 && count<=15)
			{
			
	/* routine to display all characters, that can fit on the sreen,running back from the last charcter 
		 in text message*/
				
				command(0x8f-count); 
				display(message[boundary]); 
				
				boundary=boundary-1;
				count=count+1;
			}
			delay();
			p=p+1;
		}
	}
}

//-----------------------------------------------------------------------------
// Initialization Subroutines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// delay
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//	
//
// This function is used to control the scrolling speed of the text
//
//-----------------------------------------------------------------------------
	
void delay() {
	for(i=0;i<9000;i=i+1)
	{}
	}

//-----------------------------------------------------------------------------
// status
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function checks if the LCD is busy.
//
//-----------------------------------------------------------------------------
			
void status()
{
  EN = 0 ; 
  RS = 0 ;
  RW = 1 ; // read mode of LCD
  Dt = 0xFF ; // configuring port pins as inputs
  EN = 1 ;
  while (P2_7==1) {}	; // wait if the lcd uC is busy
  EN=0 ;
}

//-----------------------------------------------------------------------------
// command
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
//	 1)   Unsigned char a - 8 bit command value to the LCD 
//
// This function transmits the desired command to the LCD uC
//
//-----------------------------------------------------------------------------
	
void command ( unsigned char a )
{
  status(); // checks the status of the LCD
  EN = 0; 
  RS = 0;
  RW = 0; // write mode of LCD
  EN = 1; 
  Dt = a; // command transfered to port pins
  EN = 0;
}

//-----------------------------------------------------------------------------
// display
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
//	 1)   Unsigned char a -  8 bit ascii value (hex value) of the character to be displayed
//
// This function transmits the ascii value of the character to be displayed ti LCD uC
//
//-----------------------------------------------------------------------------

void display (unsigned char a)
{ 
	status(); // checks the status of the LCD
	EN = 0;
	RS = 1;
	RW = 0; // write mode of LCD
	EN = 1;
	Dt = a; // 8 bit hex value of character transfered to port pins
	EN = 0;
}