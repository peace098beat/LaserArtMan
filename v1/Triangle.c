//
//16F819 A/D??LCD???
//XC8??????
//
#include <stdlib.h>
#include <xc.h>
#include <htc.h>               // delay?
//#include <math.h>

#define _XTAL_FREQ  20000000 // 20MHz   8000000    // delay?(?????MHz????)

#define A_pin PORTBbits.RB4 // Rotary Enc
#define B_pin PORTBbits.RB5 // Rotary Enc
#define LED PORTBbits.RB2
#define PWM PORTBbits.RB3


//servo program 
#define TMR0set	10 //255

// T/10[ms]
#define	T20ms	1000	// PWM 20ms(50Hz)

#define maxangle0 135	// 1350us	2.1ms = 2100us
#define minangle0  45	// 450us	0.9ms = 900uss

#define maxgain 100 // 100us
#define mingain 0 // 0us

#define maxoffset 145 //us
#define minoffset  55 //us

#define maxfreq 500
#define minfreq 1


int	maxangle1;
int minangle1;

int Dsu=15;		//Weight Average Number
int Gain=20;
int WaGain;
int	Offset=30;
int WaOffset;
int Freq=1;		//serbo speed
int WaFreq;

__CONFIG(CCPRB3 & DEBUGDIS & LVPDIS & UNPROTECT & MCLRDIS & BOREN & PWRTEN & WDTDIS & HS) ;


unsigned int map(long value, int min0, int max0, int min1, int max1){
	unsigned int ui;
	value = 100*value;
	value = (value/(max0 - min0))  * (max1 - min1);
	value = value/100;
	value = value + min1;
	ui=value;
	return ui;
}


// AD
unsigned int adconv()
{
    unsigned int temp;
    GO_DONE = 1 ;
    while(GO_DONE) ;
    temp = ADRESH ;
    temp = ( temp << 8 ) | ADRESL ;
    return temp ;
}

void adconvOffset(){
	ADCON0 = 0b01000001 ;     // (AN0)
	WaOffset=WaOffset-Offset+map(adconv(), 0, 1023, minoffset, maxoffset);
	Offset=WaOffset/Dsu;
}
void adconvGain(){
	ADCON0 = 0b01001001 ;   // (AN1)
	WaGain=WaGain-Gain+map(adconv(),0,1023, mingain, maxgain);
	Gain=WaGain/Dsu;
	maxangle1=Offset+Gain;
	minangle1=Offset-Gain;
    
    if(maxangle1>maxangle0) maxangle1 = maxangle0;
    if(minangle1<minangle0) minangle1 = minangle0;
	}
void adconvFreq(){
	ADCON0 = 0b01010001 ;     // (AN2)
	WaFreq=WaFreq-Freq+map(adconv(), 0, 1023, minfreq, maxfreq);
	Freq=WaFreq/Dsu;
}


// ?????
void mywait1(int c)
{
	int i;
	for(i=0;i<c;i++){
		__delay_us(10) ;
	}
}
void mywait2(int c)
{
	int i;
		adconvOffset() ;     	// ?????(AN0)?????????  offset
		adconvGain() ;     		// ?????(AN1)?????????  Gain
		adconvFreq() ;     		// ?????(AN0)?????????  freq

	for(i=0;i<c;i++){
		__delay_us(10) ;
	}
        
}
  

  
        

//------------------ readRE ------------------------------
// ????????????????16????????
// ???????????????????????
// ???? (-1,0,1) ????
//--------------------------------------------------------

char A_Old=0;
char B_Old=0;
int readRE(void){
    int value=0;
    
    if(B_pin == 1 && A_pin == 1 && A_Old==0 && B_Old==1){
            value= -1; // ?????
    } else if(B_pin == 1 && A_pin == 1 && A_Old==1 && B_Old==0){
            value= 1; // ????
    }
    
    A_Old=A_pin;
    B_Old=B_pin;
    return value;
}

/************************************************************/  

int MAX_SKIP=500;
int RE_SUM=0;

int getSkip(){

    int skip=RE_SUM;
    RE_SUM=0;
    
    if(skip > MAX_SKIP) skip= MAX_SKIP;
    else if(skip < -1*MAX_SKIP) skip=-1*MAX_SKIP;
    
    return skip;
}


void interrupt timer0(){  
    TMR0IF = 0; // interrupt flag clear 
	TMR0=TMR0set; 
    RE_SUM += readRE(); // Update Rotary Encorder
    
    //PWM
    g_pwm_angle_sample;
    RB3=1;
    mywait1(g_pwm_angle_sample);
    RB3=0;
    mywait2(T20ms-g_pwm_angle_sample);
}


/*******************************************************************************
*  MAIN
 * PORTB
   RB0: -
   PB1: -
   PB2: LED
   PB3: PWM
   PB4: RotaryEnc_A
   PB5: RotaryEnc_B
*******************************************************************************/
void main()
{
    int j,k;
    long stp, st0;

//Initialize
     ADCON1 = 0b10000000 ;     // AN0-AN4 I/O
     TRISA  = 0b00011111 ;     // AN0-AN4 is Input
     TRISB  = 0b00110001 ;     // RB0,RB4,RB5 is Input(1). and the ather is Output(0)
     OPTION_REG = 0b00000000 ;     // RB0-RB7 is PullUP(0b0------)

//TIMER
     TMR0 = TMR0set;
     TMR0IE=1;
     GIE=1;
     
//Noise Weight MA
	WaGain=Gain*Dsu;
	WaOffset=Offset*Dsu;
	WaFreq=Freq*Dsu;
    
    int Tangle_now;
    int skip=0;
    int Direction=1;

    LED=1;
    PWM=0;

// Initialize Variables
    Tangle_now=minangle1;
    st0=minangle1*100;
    
        
    while(1) {
        
        // PWM
        RB3=1;
        mywait1(Tangle_now);
        RB3=0;
        k=T20ms-Tangle_now;	//???20ms????????
        mywait2(k);

        // delta angle
        stp=100*Gain/Freq;
        
        // Update angle
        st0=st0 + (Direction*stp);
        
        // Phase
        skip=getSkip();
        st0=st0 + (Direction*stp * skip);

        // Next Angle
        Tangle_now=st0/100;

        // 
        if(Tangle_now > maxangle1){
            Direction = -1;
            Tangle_now=maxangle1;
        }else if(Tangle_now < minangle1){
            Direction = 1;
            Tangle_now=minangle1;
        };
        
        
        
     }
}

