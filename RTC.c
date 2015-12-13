//-----------------------------------------------------------------------------
//RTC.c
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
// 2) Connect  SCL,SDA,5v and gnd to the respective pins on board.
// 3) You should see a normal working clock set to the following LCD display:
//    10:00:00
//    10|06|2k13 MON 
// 4) Press the set key and enter the set mode to set the time
//	
// Keypad numbering : With respect to the port given the first 3x3 matrix represents thr numbers from 1-9
//										Row 1 last coloumn is the set key ( refernce : port facing us, the left column is the the first coloumn )
//										Row 2 last coloumn is the number 0 
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

#include<at89c5131.h> // SFR declarations

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

Sbit ( RW , P0 , 1 ) ;
Sbit ( RS , P0 , 0 ) ;
Sbit ( EN , P0 , 2 ) ;
Sfr  ( Dt , 0xA0 ) ;  

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------te

 bit j;
 bit p;
 bit flag;

unsigned char i;
unsigned char temp;
unsigned char R_W;
unsigned long ph;

unsigned char seconds; 
unsigned char minutes;
unsigned char hours;
unsigned char day;
unsigned char date;
unsigned char month;
unsigned char year;
unsigned char stored_word;
unsigned char prsnt_word;

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void delay();
void status ();
void command ( unsigned char a );
void display (unsigned char a);
void ascii (unsigned char a);
void i2c_interrupt ();
void scan();
void back();
void check_set();
void check();
void set_mode();

//-----------------------------------------------------------------------------
// main() Routine
//-----------------------------------------------------------------------------
		
void main()
{
	command(0x38); // function set command
	command(0x0f); // display switch command
	command(0x06); // input set command
	command(0x01); // clear screen command
	
	
	seconds = 0x00;  // set initial time values
	minutes=0x00;
	hours=0x10;
	day=0x07;
	date=0x16;
	month=0x06;
	year=0x13;
	 
	p = 0; // variable used to differentiate between write and read paths 
	j = 0; // variable used to either start or continue with the protocol
 
	R_W = 0; // write mode set
	
	IEN1 = IEN1 | 0X02; // i2c interrupt enabled
	EA = 1; // Enabling all interrupts
	
  SSCON = SSCON | 0x81; // set bit frequency to 100kHz
	SSCON = SSCON | 0x40; // synchronous serial enable bit
	SSCON = SSCON & 0xc5; // clear start,stop and serial interrupt fla
	
	while(1)
		{
		if(j==0) 
			{
			SSCON = SSCON | 0x20; // Set to send a START condition on the bus
			j = 1; // set to enter loop
			}
		while(j==1) {} // further protocol on interruption
		}
}

//-----------------------------------------------------------------------------
// Initialization Subroutines
//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------
// ascii
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
//	 1)   Unsigned char a - 8 bit value read from the DS1307 registers
//
// This function displays the BCD value read from the DS1307 registers
//
//-----------------------------------------------------------------------------

void ascii (unsigned char a)
{ 
	temp = a & 0xf0; // masking the lower-order nibble
	temp = (temp>>4); // bit shift to find the value of high-order nibble
	display(temp+0X30); // display the high-order nibble of byte
				
	temp = a & 0x00f; // masking the high-order nibble
	display(temp+0X30); // display lower-order nibble of byte
}

//-----------------------------------------------------------------------------
// delay
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
//	 1)   Unsigned char a - 8 bit value to be multiplied to 10ms delay
//
// This function produces a delay of multiples of 10ms
//
//-----------------------------------------------------------------------------

void delay (unsigned char a)
{
	for(ph=0;ph<(300*a);ph++) {}
}

//-----------------------------------------------------------------------------
// back
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
//
// This function eliminates the key press and held case
//
//-----------------------------------------------------------------------------

void back()
{
  P3=0xf0;  // configuring higher nibble as input and lower nibble as output
  while ( P3 != 0xf0 ); // wait till no key pressed
	P3 = 0x0f; // configuring lower nibble as input and higher nibble as output
	while (P3 != 0x0f ); // wait till no key pressed
	
	delay(10); // debouncing time
	
	P3=0xf0; // repeat the process
  while ( P3 != 0xf0 );
	P3 = 0x0f;
	while (P3 != 0x0f );
}

//-----------------------------------------------------------------------------
// scan
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//	 
//
// This function waits till a key is pressed
//
//-----------------------------------------------------------------------------
		
void scan()
{ 
	P3=0x0f; // configuring lower nibble as input and higher nibble as output
	while(P3==0x0f); // wait till some key is pressed
	stored_word=P3; // save the row status pressed
	
	P3=0xf0; // configuring lower nibble as input and higher nibble as output
	while(P3==0xf0); // wait till some key is pressed
	stored_word = (stored_word | P3); // save the coloumn status pressed
				
	delay(1); // debouncing time
				
	P3=0x0f; // repeat the above procedure and save the keypad status in prsnt_word
	while(P3==0x0f);
	prsnt_word=P3;
			
	P3=0xf0;
	while(P3==0xf0);
	prsnt_word = (prsnt_word | P3);
		
 	if(stored_word==prsnt_word) // check if the same key pressed  before and after debouncing time
	{
	flag=1; // valid input
	}				
}

//-----------------------------------------------------------------------------
// check_set
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//	 
//
// This function decides whether or not to enter the set mode
//
//-----------------------------------------------------------------------------

void check_set() 
{
	if(stored_word==0x7e && flag==1)
	set_mode(); // if set key pressed enter set mode
	else
	return; // if set key not pressed return back to normal RTC
}

//-----------------------------------------------------------------------------
// check
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//	 
//
// This function identifies the key pressed with the corresponding number
//-----------------------------------------------------------------------------

void check() 
{
	switch(stored_word)
		{
		case 0xee :
		{
		stored_word = 1;
	  break;
		}
		case 0xde :
		{
		stored_word = 2;
	  break;
		}
		case 0xbe :
		{
		stored_word = 3;
	  break;
		}
		case 0xed :
		{
		stored_word = 4;
	  break;
		}
		case 0xdd :
		{
		stored_word = 5;
	  break;
		}
		case 0xbd :
		{
		stored_word = 6;
	  break;
		}
		case 0xeb :
		{
		stored_word = 7;
	  break;
		}
		case 0xdb :
		{
		stored_word = 8;
	  break;
		}
		case 0xbb :
		{
		stored_word = 9;
	  break;
		}
		case 0x7d :
		{
		stored_word = 0;
		break;
		}
	}
}

//-----------------------------------------------------------------------------
// set_mode
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//	 
//
// This function allows the user to enter set mode and set the calender
//
//-----------------------------------------------------------------------------

void set_mode()
{
	command(0x01); // clear screen command
	command(0x81); // set cursor 
	display('S');  // display SET MODE
	display('E');
	display('T');
	display(' ');
	display('M');
	display('0');
	display('D');
	display('E');
	back(); // wait for the user to leave the set key
	
  delay(100); // wait for 1 sec
	
	command(0x01); // clear screen command
	command(0x81); // set cursor
	
	for(i=0;i<7;i=i+1)
	{
		P3=0xff;
		flag=0; // high flag indicates a valid keypress
		while(flag==0)
		{
		scan(); // wait till key is pressed
		}
		check(); // check which key has been pressed
		display(stored_word+0x30); // diplay the key number pressed
		back(); // check if the key is held pressed
		temp=stored_word; // store the higher BCD digit to update the time register
		temp=temp<<4;
		
		delay(10);
	 
		P3=0xff;
		flag=0;
		 
		while(flag==0)
		{
		scan(); // wait till key is pressed
		}
		check(); // check which key has been pressed
		display(stored_word+0x30); // diplay the key number pressed
		back();  // check if the key is held pressed
		temp=temp+stored_word; // store the lower BCD digit to update the time register
  
		 
		if(i==0)
		{
		hours=temp; // update hours register 
		display(':');
		}
		else if ( i==1)
		{
		minutes=temp; // update minutes register
		display(':');
		}		
		else if ( i==2)
		{	
		seconds=temp; // update seconds register
		command(0xc1); // set cursor position
		}
		else if ( i==3)
		{	
		date=temp; // update dates register
		display('|');
		}
		else if ( i==4)
		{
		month=temp; // update months register
		display('|');
		display('2');
		display('k');
		}	
		else if ( i==5)
		{	
		year=temp; // update year register
		display(' ');	
		}
		else if ( i==6)
		day=temp; // update day register
	}		 
	p=0; // set the write registers path
	i=0;
}
	
//-----------------------------------------------------------------------------
// Interrupt Service Routines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//i2c_interrupt
//-----------------------------------------------------------------------------
//
// This routine decides further protocol to be executed
//
//-----------------------------------------------------------------------------

void i2c_interrupt () interrupt 8 
{
	switch(SSCS)
		{
   	case (0x08) : // start condition has been transmitted
				{ 
				SSDAT = 0XD0 + R_W; // slave address (SLA) + write (W) transmitted
				SSCON &= 0XE7; // clear interrupt and stop flag
				break;
				}
		case (0x20) : // SLA+W has been transmitted NOT ACK has been received 
			  {
				SSCON = 0Xd1; // stop condition will be transmitted and interrupt flag cleared
			  j=0; // escape loop and start condition transmitted ( in main )
			  i=0; 
				break;
			  }
		case 0x18 : // SLA+W has been transmitted ACK has been received
			  {
				SSDAT = 0x00; // set pointer address to 00h
			  SSCON &= 0Xc7; // Data byte will be transmitted
				break;
			  }
		case 0x28 : // data byte has been transmitted ACK has been received
				{
				if (p==0) // enter when pointer address set to write
					{ 
					if (i==0)
					{ 
					SSDAT = seconds; // setting seconds to 00
					SSCON &= 0xc7; // data byte will be transmitted
					}
					else if (i==1)
					{ 
					SSDAT = minutes; // setting minutes to 00
					SSCON &= 0xc7; // data byte will be transmitted
					}
					else if (i==2)
					{ 
					SSDAT = hours; // setting hours to 10
					SSCON &= 0xc7; // data byte will be transmitted
					}
					else if (i==3)
					{ 
					SSDAT = day; // setting day to 01
					SSCON &= 0xc7; // data byte will be transmitted
					}
					else if (i==4)
					{ 
					SSDAT = date; // setting date to 10
					SSCON &= 0xc7; // data byte will be tranmitted
					}
					else if (i==5)
					{ 
					SSDAT = month; // setting month to 06
					SSCON &= 0xc7; // data byte will be transmitted
					}
					else if (i==6)
				  { 
					SSDAT = year; // setting year to 2k'13'
					SSCON &= 0xc7; // data byte will be transmitted
					}
					else if (i==7)
					{ 
					SSDAT = 0X10; // using control register to enable a 1 Hz square wave output 
					SSCON &= 0xc7; // data byte will be transmitted
					}
					else if(i==8) 
					{
					SSCON = 0xd1; // stop condition will be transmitted and interrupt flag cleared 
					j=0; // escape loop and start condition transmitted ( in main )
					p=1; 
					}
					i=i+1;
				}
			  if ( p==1 && j==1 ) // enter when pointer address set to read
					{ 
					R_W = 0x01 ; // set read mode
					SSCON = 0xe1; // repeated start will be transmitted
					}
			  break;
				}
		case 0X30 : // data byte has been transmitted and NOT ACK has been received
		    { 
			  SSCON = 0Xd1; // stop condition will be transmitted and interrupt flag cleared
			  j=0; // escape loop and send start condition ( in main )
			  i=0;
			  break;
		    }
		case 0x10 : // repeated start condition has been transmitted
		    { 
			  SSDAT = 0XD0 + R_W; // slave address (SLA) + read (R) transmitted
			  SSCON &= 0XE7; // clear interrupt and stop flag 
			  break;
		    }
		case 0X40 : // SLA+R has been transmitted ACK has been transmitted
	 		  { 
			  i=0;
			  SSCON = 0xC5; // data byte will be received and ACK will be returned
			  break;
		    }
		case 0X48 : // SLA+R has been transmitted NOT ACK has been transmitted
		    { 
			  SSCON &= 0Xd1; // stop condition will be transmitted and interrupt flag cleared
			  j=0; // escape loop and send start condition ( in main )
			  i=0;
			  break;
		    }
		case 0x50 : // data byte has been received, ACK has been returned
				{	 
				if(i==0)
					{
					seconds = SSDAT; // read seconds register
					SSCON = 0xc5; // data byte will be received and ACK will be returned
					}
				if(i==1)
					{
					minutes = SSDAT; // read minutes register
					SSCON = 0xc5; // data byte will be received and ACK will be returned
					}
				if(i==2)
					{
					hours = SSDAT; // read hours register
					SSCON = 0xc5; // data byte will be received and ACK will be returned
					}
				if(i==3)
					{
					day = SSDAT; // read days register
					SSCON = 0xc5; // data byte will be received and ACK will be returned
					}
				if(i==4)
					{
					date = SSDAT; // read date register
					SSCON = 0xc5; // data byte will be received and ACK will be returned
					}
				if(i==5)
					{
					month = SSDAT; // read months register
					SSCON &= 0xc3; // data byte will be received and NOT ACK will be returned
					}
		    i=i+1;
				break;
				}
				
		case 0X58 : // data byte has been received,NOT ACK has been returned
		    {
				year = SSDAT ; // read years register 
			  
				command(0x81); // set cursor position ( first line )
			  ascii(hours);  // display data from hours register
			  display(':');
				ascii(minutes); // display data from minutes register
			  display(':');
				ascii(seconds); // display data from seconds register
			  
				command(0xc1); // set cursor position ( second line )
			  ascii(date);  // display data frm date register
			  display('|');
			  ascii(month); // display data frm months register
			  display('|');
			  display('2');
			  display('k');
			  ascii(year); // display data frm years register ( format 2k13 )
			
			  command(0xcc); // set cursor position 
			  switch(day)
					{ 
					case 0x01 : {display('M'); // based on the days register-display the day in characters
						           display('0');
						           display('N');break;}
					case 0x02 : {display('T');
						           display('U');
						           display('E');break;}
				  case 0x03 : {display('W');
						           display('E');
						           display('D');break;}											 
					case 0x04 : {display('T');
						           display('H');
						           display('U');break;}	           
			    case 0x05 : {display('F');
						           display('R');
						           display('I');break;}
					case 0x06 : {display('S');
						           display('A');
						           display('T');break;}
					case 0x07 : {display('S');
						           display('U');
						           display('N');break;}	
           }

				SSCON = 0xd1; // stop condition will be transmitted and interrupt flag cleared
				j = 0; // escape loop and send start condition ( in main )
			  i = 0; 
				R_W = 0; // set write mode
					 
				P3=0xff;		 
        flag=0;										
				
				P3=0x0f; // configuring lower nibble as input and higher nibble as output
				delay(1);
				stored_word=P3; // save the row status pressed
				P3=0xf0;  // configuring higher nibble as input and lower nibble as output
			  stored_word = (stored_word | P3); // save the coloumn status pressed			
				
				delay(1); // debouncing time
				
				P3=0x0f; // repeat the same process
				delay(1);
				prsnt_word=P3;
			
				P3=0xf0;
			  prsnt_word = (prsnt_word | P3);
				if(stored_word==prsnt_word)
					{
					flag=1; // check if  valid key pressed  before and after debouncing time
					}	
      	check_set();	// check if the set key is pressed
				break;
				}	
		}		
}
