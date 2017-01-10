/******************************************************
 * Copyright 2011 - Robert MacCurdy. All rights reserved
******************************************************/

#ifndef UART_HANDLER_H_
#define UART_HANDLER_H_
#include "utils.hpp"

#define RX_BUFF_SZ 60	//unsigned character buffer size (bytes)
#define TX_BUFF_SZ 60	//unsigned character buffer size (bytes)

/*********************************************
 * Set the device type
*********************************************/
//#define __MSP430F2274__
/********************************************/

/**********************************************
 USE OF THIS MODULE:
This UART handler is designed to run unobtrusively in the background. User code must periodically
check if data are available with the following test:

	if(UART_RX_BUFFER.DataAvail > 0)
	{
 	 	 ParseRXCmd();	//if we have bytes in the buffer, attempt to parse
	}

Of course the test can be done on a number larger than zero


*********************************************/


/**********************************************
 * Circular UART Buffer object that allows easy pointer-based accesses from
 * circular byte-buffers using ++, -- operators and manages
 * wrap-around. It also is ISR safe, allowing a separate parser routine
 * safely access data. This is designed to act as a receive buffer that an
 * ISR is continuously filling with data.
 *********************************************/ 
class Circ_UART_Buf
{
	private:
	unsigned char *start;	//stores the starting address of the circular buffer for buffer resets 
	
	public:	
	int NonISRbytecnt;	//The LENGTH IN BYTES OF VALID DATA in circular buffer object
	int ISRbytecnt;		//indicates number of new bytes that have been added to the Buffer by the ISR. Can be used only to test for new data.
	iterator<unsigned char> NonISRPtr;		//Non-ISR object POINTS TO THE START OF VALID DATA in the Buffer
	iterator<unsigned char> ISRPtr;	//ISR's object points to the next empty space in the Buffer (where next Rx'd byte will be written). Don't use or access outside of the ISR!	

	//Pass in the char array that is allocated statically, as well as the number of
	//bytes allocated.	
	Circ_UART_Buf(){}
	
	Circ_UART_Buf(unsigned char *s, int L)
	{
		unsigned short GIE_state = Disable_Int();
		start=s;
		NonISRPtr.init(s,L);
		ISRPtr.init(s,L);
		NonISRbytecnt=0;
		ISRbytecnt=0;
		Restore_Int(GIE_state);
	}
	
	//Called to update the Object's data after an ISR has added one or more bytes to the buffer. Call this
	//after a test of ISRbytecnt>0. This fn just updates the new data count that's used by the rest of the system.
	void UpdateBuf(void)
	{
		unsigned short GIE_state = Disable_Int();
		NonISRbytecnt+=ISRbytecnt;
		ISRbytecnt=0;
		Restore_Int(GIE_state);
	}
	
	//Must be called periodically to check if data are available in the buffer
	//returns the number of bytes that are available to be read in the buffer
	int DataAvail(void)
	{
		if(ISRbytecnt > 0) UpdateBuf();	//if the ISR added data to the UART buffer, update the object...
		return NonISRbytecnt;	//and return the number of bytes in the buffer
	}

	//
	void ResetBuf(void)
	{
		unsigned short GIE_state = Disable_Int();	//pause interrupts to safely modify the vals used by ISR
		NonISRPtr.itr=start;
		ISRPtr.itr=start;
		NonISRbytecnt=0;
		ISRbytecnt=0;
		Restore_Int(GIE_state);
	}
	void FlushNChars(int n)
	{
		NonISRPtr.adv(n);
    	NonISRbytecnt-=(n);
    	if (NonISRbytecnt<0) NonISRbytecnt=0;
	}

};
	

// UART function prototypes
void Init_UART(void);
void tx_byte_to_slave(unsigned char Byte);
void tx_bytes_to_slave(unsigned char *ByteArr, int Count);
//void ParseRXCmd(void);
void tx_string_to_slave(char *Tx_string);
signed char EnoughBytes(void);
bool ValidChkSum(int numdatbytes);

/******************************************************
 * INTERRUPT DECLARES - declare ISRs and link their handlers
 ******************************************************/
__interrupt void USCIAB0RX_ISR(void);
__interrupt void USCIAB0TX_ISR(void);
/******************************************************/



#endif /*UART_HANDLER_H_*/
