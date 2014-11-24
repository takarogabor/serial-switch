/* 
 * File:   main.c
 * Author: takaro gabor
 *
 * 
 */

/*
BOREN =	Brown-out Detect Enable bit
OFF	BOD disabled
ON	BOD enabled
*/
#pragma config BOREN = OFF

/*
CPD =	Data EE Memory Code Protection bit
OFF	Data memory code protection off
ON	Data memory code-protected
*/
#pragma config CPD = OFF

/*
FOSC =	Oscillator Selection bits
EXTRCCLK	RC oscillator: CLKOUT function on RA6/OSC2/CLKOUT pin, Resistor and Capacitor on RA7/OSC1/CLKIN
INTOSCCLK	INTOSC oscillator: CLKOUT function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN
XT	XT oscillator: Crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN
LP	LP oscillator: Low-power crystal on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN
EXTRCIO	RC oscillator: I/O function on RA6/OSC2/CLKOUT pin, Resistor and Capacitor on RA7/OSC1/CLKIN
ECIO	EC: I/O function on RA6/OSC2/CLKOUT pin, CLKIN on RA7/OSC1/CLKIN
INTOSCIO	INTOSC oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN
HS	HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN
*/
//#pragma config FOSC = INTOSCIO
#pragma config FOSC = HS

/*
MCLRE =	RA5/MCLR/VPP Pin Function Select bit
OFF	RA5/MCLR/VPP pin function is digital input, MCLR internally tied to VDD
ON	RA5/MCLR/VPP pin function is MCLR
*/
#pragma config MCLRE = ON

/*
WDTE =	Watchdog Timer Enable bit
OFF	WDT disabled
ON	WDT enabled
*/
#pragma config WDTE = OFF

/*
CP =	Flash Program Memory Code Protection bit
OFF	Code protection off
ON	0000h to 07FFh code-protected
*/
#pragma config CP = OFF

/*
LVP =	Low-Voltage Programming Enable bit
OFF	RB4/PGM pin has digital I/O function, HV on MCLR must be used for programming
ON	RB4/PGM pin has PGM function, low-voltage programming enabled
*/
#pragma config LVP = OFF

/*
PWRTE =	Power-up Timer Enable bit
OFF	PWRT disabled
ON	PWRT enabled
*/
#pragma config PWRTE = OFF

//#pragma CLOCK_FREQ 4000000  // config clock to 4mhz.

#define _XTAL_FREQ 4000000

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

/*
 * RB1/RX
 * RB2/TX
 *
 Bluetooth modul:
 * The factory settings of UART are as follows: 
 * - Baud rate: 19200 bps
 * - Data bit: 8
 * - Parity: none
 * - Stop bit: 1
 * - Flow control: H/W or none
 * - Others: Please refer to AT Command Sets.
 *
Follow these steps when setting up an Asynchronous
Reception:
1. TRISB<1> and TRISB<2> should both be set to
    ?1? to configure the RB1/RX/DT and RB2/TX/CK
    pins as inputs. Output drive, when required, is
    controlled by the peripheral circuitry.
2. Initialize the SPBRG register for the appropriate
    baud rate. If a high-speed baud rate is desired,
    set bit BRGH. (Section 12.1 ?USART Baud
    Rate Generator (BRG)?).
3. Enable the asynchronous serial port by clearing
    bit SYNC and setting bit SPEN.
4. If interrupts are desired, then set enable bit
    RCIE.
5. If 9-bit reception is desired, then set bit RX9.
6. Enable the reception by setting bit CREN.
7. Flag bit RCIF will be set when reception is
    complete and an interrupt will be generated if
    enable bit RCIE was set.
8. Read the RCSTA register to get the ninth bit (if
    enabled) and determine if any error occurred
    during reception.
9. Read the 8-bit received data by reading the
    RCREG register.
10. If an OERR error occurred, clear the error by
    clearing enable bit CREN.
 */

volatile unsigned char state;

//soros porton vett adatok buffere, körbe forog ahogy jönnek az adatok
volatile unsigned char rxBuf[4];
volatile int rxBufIndx = 0;

unsigned char parBe[4] = "be\r\n";
unsigned char parKi[4] = "ki\r\n";
unsigned char parSt[4] = "st\r\n";
unsigned char parOk[4] = "ok\r\n";

volatile int send;
volatile unsigned char* toSend;


int isInBuf(unsigned char* parancs) {
    int i = 0;
    int bufIndx = rxBufIndx;
    for(i = 0; i < 4; i++) {
        if(bufIndx > 3) {
            bufIndx = 0;
        }
        if(parancs[i] != rxBuf[bufIndx]) {
            return 0;
        }
        bufIndx++;
    }
    return 1;
}

void interrupt megszakitas() {
    unsigned char ch = 0;
    int i = 0;
    int j = 0;
    while(PIR1bits.RCIF) {
        ch = RCREG;
        if(RCSTAbits.FERR == 0) {//ha nincs frame hiba (jo vagyis 1 a stop bit)
            //betesszuk a vett karaktert a vételi pufferbe
            if(rxBufIndx > 3) {
                rxBufIndx = 0;
            }
            rxBuf[rxBufIndx] = ch;
            rxBufIndx++;
            //adatok formátuma: két karakter a parancs plusz a vége: "\r\n",
            //tehát ha '\n' karakter az aktuálisan vett akkor megnézzük a puffert hogy egy parancs van-e benne
            if(ch == '\n') {
                i = rxBufIndx;
                j = 0;
                if(isInBuf(parKi)) {
                    state = 0;
                    PORTBbits.RB5 = 0;
                    toSend = parOk;
                    send = 1;
                } else if(isInBuf(parBe)) {
                    state = 1;
                    PORTBbits.RB5 = 1;
                    toSend = parOk;
                    send = 1;
                } else if (isInBuf(parSt)) {
                    if(state == 0) {
                        toSend = parKi;
                    } else {
                        toSend = parBe;
                    }
                    send = 1;
                }
            }
//            TXREG = ch;
//            if(ch == '1') {
//                state = 1;
//                PORTBbits.RB5 = 1;
//            } else if(ch == '0') {
//                state = 0;
//                PORTBbits.RB5 = 0;
//            }
        }
    }
}

int main(int argc, char** argv) {
    unsigned long i;
    state = 0;
    send = 0;
    PORTA = 0b00000000;
    PORTB = 0b00000000;

    TRISA = 0b00000000;
    TRISB = 0b00000110;

    //komparator ki
    CMCON = 0b00000111;
    
    //10 Megás kaviccsal igy jonne ki: BRGH = 1:
    //x = (10 000 000 / 19200 / 16) - 1 = 31.55  
    //br = 10 000 000 / (16*(32 + 1)) = 18939.39
    //hiba = -1.359375

    //4Mega BRGH = 1:
    //x = (4 000 000 / 19200 / 16) - 1 = 12.0208
    //br = 4 000 000 / (16*(12 + 1)) = 19230.769
    //hiba = 0.16 %
    SPBRG = 12;


    //TXSTA ? TRANSMIT STATUS AND CONTROL REGISTER
    TXSTAbits.TX9 = 0;      //1 = Selects 9-bit transmission, 0 = Selects 8-bit transmission
    TXSTAbits.TXEN = 0;     //Transmit disabled
    TXSTAbits.SYNC = 0;     //Asynchronous mode
    TXSTAbits.BRGH = 1;     //0 = Low speed, 1 = High speed
                            //TX9D: 9th bit of transmit data. Can be parity bit

    //RCSTA ? RECEIVE STATUS AND CONTROL REGISTER
    RCSTAbits.SPEN = 1;     //Serial Port Enable bit
    RCSTAbits.RX9 = 0;      //1 = Selects 9-bit reception, 0 = Selects 8-bit reception
            //SREN: Single Receive Enable bit Asynchronous mode: Don?t care
    RCSTAbits.CREN = 1;     //1 = Enables continuous receive, 0 = Disables continuous receive

    TXSTAbits.TXEN = 1;     //Transmit enabled

    /*
        The actual interrupt can be
        enabled/disabled by setting/clearing enable bit RCIE
        (PIE1<5>). Flag bit RCIF is a read-only bit, which is
        cleared by the hardware. It is cleared when the RCREG
        register has been read and is empty.
     */
    PIE1bits.RCIE = 1;      //serial receive interrupt enable
    INTCONbits.PEIE = 1;    //perif interrupt enable
    INTCONbits.GIE = 1;     //general interrupt enable

    while(1) {
        //ha tulcsordulas hiba van valamier ujra kell inditani a vetelt
        if(RCSTAbits.OERR) {
            RCSTAbits.CREN = 0;
            RCSTAbits.CREN = 1;
        }

        if(send) {
            for(i = 0; i < 4; i++) {
                while(!TXSTAbits.TRMT){
                    //Waiting for Previous Data to Transmit completly
                }
                TXREG = toSend[i];
            }
            send = 0;
        }

        //villogtatjuk a ledet allapottol fuggoen gyorsabban vagy lasabban
        PORTBbits.RB4 = 1;
        PORTAbits.RA0 = 1;
        for(i = 0; i<25; i++) {
            __delay_ms(10);
        }
        if(state == 0) {
            for(i = 0; i<25; i++) {
                __delay_ms(10);
            }
        }
        PORTBbits.RB4 = 0;
        PORTAbits.RA0 = 0;
        for(i = 0; i<50; i++) {
            __delay_ms(10);
        }
        if(state == 0) {
            for(i = 0; i<25; i++) {
                __delay_ms(10);
            }
        }
        
    }
    return (EXIT_SUCCESS);
}
