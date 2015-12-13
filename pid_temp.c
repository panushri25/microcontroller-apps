//-----------------------------------------------------------------------------
//pid_start.c
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
// 2) Connect the closed loop circuit as given in the documentation
// 3) You should see the tempeterature trying to attain the set point based on the given kp value
//  
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

#include<at89c5131.h>

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

Sbit ( RW , P0 , 1 ) ;
Sbit ( RS , P0 , 0 ) ;
Sbit ( EN , P0 , 2 ) ;
Sfr ( Dt , 0xA0 ) ;    

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

float kp;
unsigned int p;
unsigned int j;
unsigned long i;

unsigned char temp;
unsigned char dec;
unsigned long duty;
unsigned char count;
unsigned long spid;
unsigned long desired_val;

signed long error;
signed long val;
unsigned long t_spid;

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void delay();
void status ();
void command ( unsigned char a );
void display (unsigned char a);
void spi_interrupt ();

//-----------------------------------------------------------------------------
// main() Routine
//-----------------------------------------------------------------------------
	
void main ()
{
	command(0x38); // function set command
	command(0x0f); // display switch command
	command(0x06); // input set command
	
	desired_val=0x050; // set the desired value of temperature required ( in terms of hexadecimal output from ADC )
	
	P3=0x0f; // set P3 as input
	
	CCAPM1 = 0x42; // set module 1 of PCA timer in PWM mode
  CCAP1L = 0X90; // set the initial duty cyle
	CCAP1H = CCAP1L;
	CMOD = 0x06; // set source of PWM generator, here external function generator
	CCON = 0X40; // PCA Counter Run control bit
	
	kp = 5*( P3 & 0x0f); // kp is a multiple of the input obtained at port P3
  
	t_spid=0;
	count = 0;

	while(1)
	{
		P1_0=1; // disable chip select
		
		SPCON |= 0x10; // Master mode 
		P1_1=1; // enable master 
		SPCON |= 0x82; // Fclk Periph/128 
		SPCON &= ~0x08; // CPOL=0; transmit mode example 
		SPCON |= 0x04; // CPHA=1; transmit mode example 
		IEN1 |= 0x04; // enable spi interrupt 
		SPCON |= 0x40; // run spi 
		EA=1; // enable interrupts 

		P1_0=0; // enable chip select
	
		i=0;
		j=0;
		
		SPDAT=0X01; // send start bit
		while(i==0) {}
		i=0;
		SPDAT=0X80; // send the configuration bits of MCP3008 
		while (i==0) {}
		i=0;
		SPDAT=0X00; // dummy data
		while (i==0)
		{}
		
		P1_0=1; // disable chip select

		error = (desired_val-spid ); // error calculation
			
		val  =  0x90 -  ( error*kp);
		duty = 0x90 -  ( error*kp) ; // duty cyle of PWM output (0 refers to 100% duty cycle anf 0xff refers to 0% duty cycle)
		
		if( ( val < 0 ) )
		duty = 0x00; // limit duty cycle within the possible range 
		if ( duty > 0xff )
		duty = 0xff ; 
	
		CCAP1H = duty ; // set the duty cycle
		CH=0xff;
		CF=0;
		
		while(CF==0) {} // wait for duty cycle to be implemented
		
		t_spid = t_spid + spid ; // add 5 consecutive ADC values
		count = count + 1; // keeping track of number of values added
			
		while ( count == 5 )
		{
			spid = (t_spid/5); // consider the avergae of 5 consecutive values
			
			command(0x01); // clear screen command
			command(0x81); // set cursor position

				
			j=spid & 0x0f00; // display first 2 bits of the 10-bit ADC output
			j=(j>>8);
			if(j<=9)
			display(j+0X30);
			else
			display(j+55);  
				
			j=spid & 0x00f0; // display the next four bits of the 10-bit ADC output
			j=(j>>4);
			if(j<=9)
			display(j+0X30);
			else
			display(j+55);
		
			j=spid & 0x00f; // display the last four bits of 10-but ADC output
			if(j<=9)
			display(j+0X30);
			else
			display(j+55);	

			dec=(((spid*500)%1024)*10/1024); // first decimal point value of temperature reading
			spid=((spid*500)/1024); // decimal value of temperature  reading

			for(j=0;j<3;j=j+1)
			{
				command(0xc3-j); // decides position of cursor
				i=(spid%10);
				spid=(spid/10);
				display(i+0x30); // display the decimal part of temperature reading
			}
			command(0xc4); // decides postion of cursor
			display(46); // display '.'
			
			command(0xc5); // decides postion of cursor
			display(dec+0x30); // display decimal point value of temperature reading		
			
			count =0; // reset count
			t_spid = 0; // reset total sum of 5 consecutive readings
		}
	 delay();
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
// delay
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//	 
// This function produces a delay between two consecutive reading
//
//-----------------------------------------------------------------------------

void delay() 
{
	for(i=0;i<2000;i=i+1) {}
}

//-----------------------------------------------------------------------------
// Interrupt Service Routines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//spi_interrupt
//-----------------------------------------------------------------------------
//
// This routine stores the received value from spi communication
//
//---------------------------------------------------------------------------

void spi_iterrupt () interrupt 9
{
	i=SPSTA; // read SPSTA : necessary to clear the flag
	if(j==0)
	spid=SPDAT; // read dummy data received : necessary to clear the flag
	if(j==1)
	{
		spid=SPDAT;
		spid=spid&0x03; // read the first two MSB bits of the 10 bit ADC output
		spid=(spid<<8); // set them in MSB position
	}
  if(j==2)
	{  
		i=SPDAT; // read the rest of the ADC output
		spid=i|spid; // 10 bit ADC output
	}
	i=1;
	j=j+1;
}
