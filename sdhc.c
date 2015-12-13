#include <at89c5131.h>
#include "integer.h"
#include "ffconf.h"
#include "diskio.h"
#include <stdio.h>
	 
xdata WORD CardType;    /* MMC = 0, SDCard v1 = 1, SDCard v2 = 2 */
Sbit ( RW , P0 , 1 ) ;
Sbit ( RS , P0 , 0 ) ;
Sbit ( EN , P0 , 2 ) ;
Sfr ( Dt , 0xA0 ) ; 
	 
	

#define SD_TIME_OUT 5
#define SD_ERROR 2
 	
BYTE received_byte;
BYTE i;
BYTE p;	
ULONG j;

void status()
{
 EN=0 ;
 RS=0 ;
 RW=1 ;
 Dt=0xFF ;
 EN=1 ;
	while (P2_7==1)
	{}	;
 EN=0 ;
		return;
	}

void command ( unsigned char a )
{
status();
 EN=0;
 RS=0;
 RW=0;
 EN=1;
 Dt=a;
 EN=0;
	return;
}

void display (unsigned char a)
{ 
 status();
 EN=0;
 RS=1;
 RW=0;
 EN=1;
 Dt=a;
 EN=0;
 return;
}


void ascii (unsigned char a)
{ 
	BYTE temp;
	temp = a & 0xf0; // masking the lower-order nibble
	temp = (temp>>4);
	if(temp<=9)
			 display(temp+0X30);
				else
				display(temp+55);// bit shift to find the value of high-order nibble
                        // display the high-order nibble of byte
				
	temp = a & 0x00f; // masking the high-order nibble
	 if(temp<=9)
			 display(temp+0X30);
				else
				display(temp+55);// display lower-order nibble of byte
			}

void Delay( unsigned char a)
{
	for(j=0;j<30*a;j++) {}
}
	 
void spi_interrupt () interrupt 9
{
	j=SPSTA;
	received_byte = SPDAT;	       
	i=1;
}
		 
BYTE send_byte( BYTE tosend )
{ 	
	i=0;
	SPDAT=tosend;
	while(i==0)
	{}
 return(received_byte);		
}

BYTE SD_GetR1()
{
                  /* response will be after 1-8 0xffs.. */
      for( j=0; j<8; j++ )
	  //while(1)
		 {
			 p = send_byte( 0xff );
      if(p != 0xff)         /* if it isn't 0xff, it is a response */
         return(p);
   }
	 return(p);
 }
 

WORD SD_GetR2()
{
   idata WORD R2;
   
   R2 = ((SD_GetR1())<< 8)&0xff00;
   R2 |= send_byte( 0xff );
   return( R2 );
}

typedef union
{
   BYTE b[4];
   ULONG ul;
} b_ul;

 
BYTE send_command ( BYTE command , ULONG ThisArgument)
{ 
	b_ul Temp;
   BYTE i;

	P3_2 = 0;
  send_byte( 0xFF );
	
	//ascii(command);
	send_byte(0x40|command);
	Temp.ul = ThisArgument;
	for(j=0;j<4;j++)
 {send_byte(Temp.b[ i ]);
  //ascii(Temp.b[ i ]);
	 }
	
	if(command==CMD_GO_IDLE_STATE)
		p=0x95;
	else
		p=0xff;
  //ascii(p);
	send_byte(p);
 
  send_byte(0xff);
  return(0);
}


BYTE SD_ReadSector( ULONG SectorNumber, BYTE *Buffer )
{
   BYTE c, i;
   WORD count;


   send_command( CMD_READ_SINGLE_BLOCK, SectorNumber << 9 );
   c = SD_GetR1();
   i = SD_GetR1();
   count = 0xFFFF;
   
   
   while( (i == 0xff) && --count)
      i = SD_GetR1();
 
  
   if(c || i != 0xFE)
      return( 1 );

  
   for( count=0; count<512 ; count++)      
      *Buffer++ = send_byte(0xFF);
   
   
   send_byte(0xFF);          
   send_byte(0xFF);
   
  
  P3_2=1;

   return( 0 );
}

BYTE SD_WriteSector( ULONG SectorNumber, BYTE *Buffer )
{
   BYTE i;
   WORD count;

  
  send_command( CMD_WRITE_SINGLE_BLOCK, SectorNumber << 9 );
   i = SD_GetR1();

   
  send_byte( 0xFE );

 
   for( count= 0; count< 512; count++ )
   {
      send_byte(*Buffer++);
   }
 
   send_byte(0xFF);          
   send_byte(0xFF);
   
   
   while( send_byte( 0xFF ) != 0xFF)
  
   send_byte( 0xFF );
   

  P3_2=1;
  send_byte( 0xFF );
   return( 0 );
}

BYTE SD_WaitForReady()
{
   BYTE i;
   WORD j;
   send_byte( 0xFF );
   
   j = 500;
   do
   {
      i = send_byte( 0xFF );
      Delay( 1 );
   } while ((i != 0xFF) && --j);
   
   return( i );
}


BYTE SD_Init(void)
{
	WORD CardStatus; // R2 value from status inquiry...
   WORD Count;      // local counter
   char temp;
   // Global CardType - b0:MMC, b1:SDv1, b2:SDv2

		P3=0x00;
   /* initial speed is slow... */
  	SPCON |= 0x10; /* Master mode */
	P1_1=1; /* enable master */
	SPCON |= 0x03; /* Fclk Periph/16 */
	SPCON |= 0x08; /* CPOL=0; transmit mode example */
	SPCON |= 0x04; /* CPHA=1; transmit mode example */
	IEN1 |= 0x04; /* enable spi interrupt */
	SPCON |= 0x40; /* run spi */
	EA=1; /* enable interrupts */
	
 Delay(2);
	
	/* disable SPI chip select... */
  P3_2=1;

   /* fill send data with all ones - 80 bits long to   */
   /* establish link with SD card this fulfills the    */
   /* 74 clock cycle requirement...  */

	
	 for(Count=0;Count<10;Count++)
      send_byte( 0xFF );
   
 Delay(10);
   /* enable the card with the CS pin... */
   P3_2=0;

   /* ************************************************ */
   /* SET SD CARD TO SPI MODE - IDLE STATE...          */
   /* ************************************************ */
   Count = 1000;     // one second...
   CardType = 0;
   

   /* wait for card to enter IDLE state... */
   do
		 {
      Delay(1);
      send_command( CMD_GO_IDLE_STATE, 0 );
		 temp=SD_GetR1();
	
		// while(1);
		//ascii(temp);
   } while((temp != IDLE_STATE) && (--Count));  

   /* timeout if we never made it to IDLE state... */
   if( !Count )
      return( SD_TIME_OUT );

   /* ************************************************ */
   /* COMPLETE SD CARD INITIALIZATION                  */
   /* FIGURE OUT WHAT TYPE OF CARD IS INSTALLED...     */
   /* ************************************************ */
   Count = 2000;     // two seconds...
   
   /* Is card SDSC or MMC? */
   send_command( CMD_APP_CMD, 0 );
   send_command( ACMD_SEND_OP_COND, 0 );
   if( SD_GetR1() <= 1 )
   {
      CardType = 2;
   }
   else
   {
      CardType = 1;
   }
   
   /* wait for initialization to finish... */
   do
   {
      Delay(1);
      if( CardType == 2 )
      {
         send_command( CMD_APP_CMD, 0 );
         send_command( ACMD_SEND_OP_COND, 0 );
      }
      else
      {
         send_command( CMD_SEND_OP_COND, 0 );
      }
   } while(SD_GetR1() && --Count);

   if( !Count )
      return( SD_TIME_OUT );

   /* ************************************************ */
   /* QUERY CARD STATUS...                             */
   /* ************************************************ */
   send_command( CMD_SEND_STATUS, 0 );
   CardStatus = SD_GetR2();

   if( CardStatus )
      return( SD_ERROR );

   /* ************************************************ */
   /* SET BLOCK SIZE...                                */
   /* ************************************************ */
   send_command( CMD_SET_BLOCKLEN, 512 );
   if( SD_GetR1() )
   {
      CardType = 0;
      return( SD_ERROR );
   }

   /* ************************************************ */
   /* SWITCH TO HIGHEST SPI SPEED...                   */
   /* ************************************************ */
   SPCON &= 0xfd;

   /* disable the card with the CS pin... */
   P3_2=1;

   /* return OK... */
   return( 0 );
}
 
 
void main()
{
	command(0x38);
	 command(0x0f);
	 command(0x06);
	command(0x01);
	command(0x80);
	
	ascii(0x95);

	p = SD_Init();
	
	switch(p)
	{
	case 0 : 
	{
		
	display(2+0x30);
		printf("Successful\r\n");
		break;
	}
	case SD_ERROR :
			{
				
	display(3+0x30);
		printf("Error\r\n");
		break;
	}
	case SD_TIME_OUT :
			{
				
	display(4+0x30);
		printf("timeout\r\n");
		break;
	}
}
while(1);
}
	