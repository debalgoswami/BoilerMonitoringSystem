#include<reg51.h>
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include <intrins.h>
#define adc_input P1
#define dataport P2
sbit servo_pin = P3^2;
sbit servo_pin_1 = P3^7;
sfr lcd_data_port=0xA0;			/* P1 port as data port */
sbit rs=P0^5;			/* Register select pin */
sbit rw=P0^6;			/* Read/Write pin */
sbit en=P0^7;			/* Enable pin */
sbit DHT11=P3^3;		/* Connect DHT11 output Pin to P2.1 Pin */
sbit rd= P3^4;
sbit wr= P3^5;
sbit intr= P3^6;
sbit sw=P3^2;
sbit PIR = P0^4;
int num[10];
float v_out,pressure;

int I_RH,D_RH,I_Temp,D_Temp,CheckSum;
void servo_delay(unsigned int us)
{
  while(us--)
  {
    _nop_();
  }
}
void delay(unsigned int count)    		/* Function to provide delay Approx 1ms with 11.0592 Mhz crystal*/
{
     int i,j;
     for(i=0;i<count;i++)
     for(j=0;j<112;j++);
}
void cct_init(void)   //initialize cct
{
    P3 = 0x03; //used for serial

}
void SerialInitialize(void)                   // INITIALIZE SERIAL PORT
{
    TMOD = 0x20;                           // Timer 1 IN MODE 2 -AUTO RELOAD TO GENERATE BAUD RATE
    SCON = 0x50;                           // SERIAL MODE 1, 8-DATA BIT 1-START BIT, 1-STOP BIT, REN ENABLED
    TH1 = 0xFD;                       // LOAD BAUDRATE TO TIMER REGISTER
    TR1 = 1;                               // START TIMER
}

void uart_tx(unsigned char serialdata)
{
    SBUF = serialdata;                        //Load Data to Serial Buffer Register
    while(TI == 0);                               //Wait Until Transmission To Complete
    TI = 0;                                                //Clear Transmission Interrupt Flag
}

void uart_msg(unsigned char *c)
{
                while(*c != 0)

                {
                     uart_tx(*c++);
                }
}
void adc_conv()
{                      
wr = 0;
delay(2);                     
wr = 1;                     
while(intr);
delay(2);
intr=1; 
}
void lcd_data_adc(unsigned int i)
{
int p;
int k=0;
while(i>0)
{
  num[k]=i%10;
  i=i/10;
  k++;
}
k--;
for (p=k;p>=0;p--)
{
dataport=num[p]+48;
rw = 0;
rs = 1;
en = 1;
delay(1);
en = 0;
}
}
void adc_read()
{
unsigned int value;                  
rd = 0;
delay(2);
value=adc_input;
v_out=(5.0/256)*value;      //divide by 256 as it's a 8 bit converter
 
pressure=((v_out/5.0)+0.09)/0.009; // convert output voltage(volt) to    KPA
delay(1);
rd=1; 
   servo_pin = 0;
   if(pressure>88){
         cct_init();
    SerialInitialize();  
    uart_msg("High Pressure");
     uart_tx(0x0d);
        //Rotate to 90 degree
   servo_pin=1;
   servo_delay(150);
   servo_pin=0;
	 cct_init();
    SerialInitialize();  
    uart_msg("Water Valve Closing");
     uart_tx(0x0d);
   delay(100);
      }
      else{
         cct_init();
    SerialInitialize();  
    uart_msg("Normal Pressure");
     uart_tx(0x0d);
	 //Rotate to 0 degree
   servo_pin = 1;
   servo_delay(50);
   servo_pin = 0;
	  cct_init();
    SerialInitialize();  
    uart_msg("Water Valve Opening");
     uart_tx(0x0d);
   delay(100);
      }
      
lcd_data_adc(pressure);                                          
}


void LCD_Command (unsigned char cmd)  /* LCD16x2 command funtion */
{
	lcd_data_port= cmd;
	rs=0;			/* command reg. */
	rw=0;			/* Write operation */
	en=1; 
	delay(1);
	en=0;
	delay(5);
}

void LCD_Char (unsigned char char_data)  /* LCD data write function */
{
	lcd_data_port=char_data;
	rs=1;			/* Data reg.*/
	rw=0;			/* Write operation*/
	en=1;   				
	delay(1);
	en=0;
	delay(5);
}

void LCD_String (unsigned char *str) /* Send string to LCD function */
{
	int i;
	for(i=0;str[i]!=0;i++)  /* Send each char of string till the NULL */
	{
		LCD_Char (str[i]);  /* Call LCD data write */
	}
}

void LCD_String_xy (char row, char pos, char *str)  /* Send string to LCD function */
{
	if (row == 0)
	LCD_Command((pos & 0x0F)|0x80);
	else if (row == 1)
	LCD_Command((pos & 0x0F)|0xC0);
	LCD_String(str);	/* Call LCD string function */

}

void LCD_Init (void)		/* LCD Initialize function */
{	
	delay(20);		/* LCD Power ON Initialization time >15ms */
	LCD_Command (0x38);	/* Initialization of 16X2 LCD in 8bit mode */
	LCD_Command (0x0C);	/* Display ON Cursor OFF */
	LCD_Command (0x06);	/* Auto Increment cursor */
	LCD_Command (0x01);	/* clear display */
	LCD_Command (0x80);	/* cursor at home position */
}

void timer_delay20ms()		/* Timer0 delay function */
{
	TMOD = 0x01;
	TH0 = 0xB8;		/* Load higher 8-bit in TH0 */
	TL0 = 0x0C;		/* Load lower 8-bit in TL0 */
	TR0 = 1;		/* Start timer0 */
	while(TF0 == 0);	/* Wait until timer0 flag set */
	TR0 = 0;		/* Stop timer0 */
	TF0 = 0;		/* Clear timer0 flag */
}

void timer_delay30us()		/* Timer0 delay function */
{
	TMOD = 0x01;		/* Timer0 mode1 (16-bit timer mode) */
	TH0 = 0xFF;		/* Load higher 8-bit in TH0 */
	TL0 = 0xF1;		/* Load lower 8-bit in TL0 */
	TR0 = 1;		/* Start timer0 */
	while(TF0 == 0);	/* Wait until timer0 flag set */
	TR0 = 0;		/* Stop timer0 */
	TF0 = 0;		/* Clear timer0 flag */
}

void Request()			/* Microcontroller send request */
{
	DHT11 = 0;		/* set to low pin */
	timer_delay20ms();	/* wait for 20ms */
	DHT11 = 1;		/* set to high pin */
}

void Response()			/* Receive response from DHT11 */
{
	while(DHT11==1);
	while(DHT11==0);
	while(DHT11==1);
}

int Receive_data()		/* Receive data */
{
	int q,c=0;	
	for (q=0; q<8; q++)
	{
		while(DHT11==0);/* check received bit 0 or 1 */
		timer_delay30us();
		if(DHT11 == 1)	/* If high pulse is greater than 30ms */
		c = (c<<1)|(0x01);/* Then its logic HIGH */
		else		/* otherwise its logic LOW */
		c = (c<<1);
		while(DHT11==1);
	}
	return c;
}

 
void main()
{
  
   unsigned char dat[20];
	 dataport=0x00;
	 adc_input=0xff;
	 P3=0x00;
   
LCD_Init(); 		/* initialize LCD */
    cct_init();
    SerialInitialize();  
    EA = 1;

    ES = 1;
 
        uart_msg("Boiler Monitoring System");
   uart_tx(0x0d);
    uart_msg("Welcome");
    uart_tx(0x0d);                                                   //next line
     uart_msg("Project by Rajat Debal Sreerag");
     uart_tx(0x0d);
   uart_tx(0x0d);
   uart_tx(0x0d);
   
     

while(1)
{

  while(1)
{
 LCD_Command(0x84);
LCD_String("PRESSURE"); 
LCD_Command (0xc5);
adc_conv();
adc_read();
LCD_String(" KPA");
  
   delay(500);
   LCD_Init(); 
   break;
 }

  while(1) {
        if(PIR == 0) {
             LCD_Command(0xc0);
            LCD_String("Flow Detected");
            delay(10);
	   
         cct_init();
    SerialInitialize();  
    uart_msg("Flow Detected");
     uart_tx(0x0d);
  
        } else {
             LCD_Command(0xc0);
            LCD_String(" Flow Undetected");
	      cct_init();
    SerialInitialize();  
    uart_msg("Flow Undetected");
     uart_tx(0x0d);
        }
	delay(100);
   LCD_Init(); 	
   break;
    }
	while(1)
	{		
		Request();	/* send start pulse */
		Response();	/* receive response */
		
		I_RH=Receive_data();	/* store first eight bit in I_RH */
		D_RH=Receive_data();	/* store next eight bit in D_RH */
		I_Temp=Receive_data();	/* store next eight bit in I_Temp */
		D_Temp=Receive_data();	/* store next eight bit in D_Temp */
		CheckSum=Receive_data();/* store next eight bit in CheckSum */
	   
	   

		if ((I_RH + D_RH + I_Temp + D_Temp) != CheckSum)
		{
			LCD_String_xy(0,0,"Error");
		}
		
		else
		{
			sprintf(dat,"Hum = %d.%d",I_RH,D_RH);
			LCD_String_xy(0,0,dat);
			    cct_init();
    SerialInitialize();  
		      uart_msg(dat);
     uart_tx(0x0d);
			sprintf(dat,"Tem = %d.%d",I_Temp,D_Temp);   
			LCD_String_xy(1,0,dat);
		     SerialInitialize();  
		      uart_msg(dat);
     uart_tx(0x0d);
		  
			LCD_Char(0xDF);
			LCD_String("C");
			memset(dat,0,20);
			sprintf(dat,"%d   ",CheckSum);
			LCD_String_xy(1,13,dat);
		}
		   servo_pin_1=0;
		if((int)I_Temp>30)
		{
		      //Rotate to 90 degree
   servo_pin_1=1;
   servo_delay(150);
   servo_pin_1=0;
	 cct_init();
    SerialInitialize();  
    uart_msg("Fuel Valve Closing");
     uart_tx(0x0d);
		   uart_tx(0x0d);   
   delay(100);
		}
		else{
		   //Rotate to 0 degree
   servo_pin_1 = 1;
   servo_delay(50);
   servo_pin_1 = 0;
	 cct_init();
    SerialInitialize();  
    uart_msg("Fuel Valve Opening");
     uart_tx(0x0d);
		   uart_tx(0x0d);   
   delay(100);
		}
		delay(500);
		LCD_Init(); 	
		break;
	}
	delay(100);
	
}
}
