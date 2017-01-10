/******************************************************
 * Copyright 2011 - Robert MacCurdy. All rights reserved
******************************************************/

#include "UART_HANDLER.hpp"
#include <stdio.h>



//RS232 UART comms objects
unsigned char RX_ser_buf[RX_BUFF_SZ ];	//allocate static storage for the RX serial buffer...
Circ_UART_Buf UART_RX_BUFFER(RX_ser_buf,RX_BUFF_SZ); //... and declare the serial comms buffer object to be used
unsigned char TX_ser_buf[TX_BUFF_SZ];	//allocate static storage for the TX serial buffer...
Circ_UART_Buf UART_TX_BUFFER(TX_ser_buf,TX_BUFF_SZ); //... and declare the serial comms buffer object to be used

/*need to add these vars to other source files by adding these lines:
extern Circ_UART_Buf UART_RX_BUFFER;
extern Circ_UART_Buf UART_TX_BUFFER;
*/

/**************************************************
//DEVICE ADDRESS MAPPING
Map register and flag names to a device-agnostic namespace
**************************************************/
#ifdef __MSP430F2274__
#define Generic_TX_IntFlag_Reg UC0IFG
#define Generic_TX_Flag_Mask UCA0TXIFG
#define Generic_RX_IntFlag_Reg UC0IFG
#define Generic_RX_Flag_Mask UCA0RXIFG
#define Generic_TX_IntEn_Reg UC0IE
#define Generic_TX_IntEn_Mask UCA0TXIE
#define Generic_RX_IntEn_Reg UC0IE
#define Generic_RX_IntEn_Mask UCA0RXIE
#define Generic_RX_Buf UCA0RXBUF
#define Generic_TX_Buf UCA0TXBUF
#define Generic_UART_Ctrl_Reg_1	UCA0CTL1
#define Generic_UART_Sel_SMCLK_MASK UCSSEL_3
#define Generic_UART_Assert_RST_MASK UCSWRST
#define Generic_UART_BR_Reg_0 UCA0BR0
#define Generic_UART_BR_Reg_1 UCA0BR1
#define Generic_UART_MC_Reg_0 UCA0MCTL
#define Generic_UART_FnSel_Reg P3SEL
#define Generic_UART_FnSel_Mask (BIT4+BIT5)
#endif
/*************************************************/

/**************************************************
Init_UART(void)
Must be called during device startup to setup the UART
objects
**************************************************/
void Init_UART(void)
{
	Generic_UART_Ctrl_Reg_1 |= Generic_UART_Assert_RST_MASK;	// **Put USCI state machine in reset**
	Generic_UART_Ctrl_Reg_1 |= Generic_UART_Sel_SMCLK_MASK; 			// choose SMCLK
	Generic_UART_FnSel_Reg |= Generic_UART_FnSel_Mask;	//select 7 enable the Rx & Tx pins as alternate functions
	Generic_UART_BR_Reg_1 = 3;		// 8MHz/(3*256+65) = 9600 baud
	Generic_UART_BR_Reg_0 = 65;
	Generic_UART_MC_Reg_0 = UCBRS_2;	// Modulation UCBRSx=2, UCBRFx=0, Oversampling disabled
	Generic_UART_Ctrl_Reg_1 &= ~Generic_UART_Assert_RST_MASK;	// **Enable USCI state machine**
	Generic_RX_IntEn_Reg |= (Generic_RX_IntEn_Mask);        	// Enable RX interrupt (TX interrupt enable is handled elsewhere)

	// Initialize the RX & TX buffer structures.
	UART_RX_BUFFER.ResetBuf();
	UART_TX_BUFFER.ResetBuf();
}
/*************************************************/

/**************************************************
tx_byte_to_slave - fn sends a single byte using USART_A0
**************************************************/
void tx_byte_to_slave(unsigned char Byte)
{
	while (!(Generic_TX_IntFlag_Reg&Generic_TX_Flag_Mask));  // TX buffer ready?
	Generic_TX_Buf  = Byte;
}
/*************************************************/

/**************************************************
tx_bytes_to_slave - fn sends Count number of bytes from ByteArr
**************************************************/
void tx_bytes_to_slave(unsigned char *ByteArr, int Count)
{
	//first move the new data into the output buffer
	int i;
	for (i=0;i<Count;i++)
	{
		*UART_TX_BUFFER.NonISRPtr.itr = ByteArr[i];
		UART_TX_BUFFER.NonISRPtr.inc();
	}
	
	//next update the ISR's count
	unsigned short GIE_state = Disable_Int();
	UART_TX_BUFFER.ISRbytecnt += Count;
	Restore_Int(GIE_state);
	
	//finally, enable the TX interrupt to signal when the UART TX buffer is ready to accept a byte
	Generic_TX_IntEn_Reg |= (Generic_TX_IntEn_Mask);
}
/*************************************************/

/**************************************************
tx_string_to_slave - function transmits a null terminated string pointed to by *Tx_string
**************************************************/
void tx_string_to_slave(char *Tx_string)
{
    while((*Tx_string) != '\0')
    {
            while (!(Generic_TX_IntFlag_Reg&Generic_TX_Flag_Mask));  // USART_A0 TX buffer ready?
            Generic_TX_Buf = *Tx_string++;               // push byte out, increment pointer
    }
}
/*************************************************/

//AN EXAMPLE OF A PARSER
///**************************************************
//ParseRXCmd - fn reads in serial commands and responds to them
//**************************************************/
//void ParseRXCmd(void)
//{
//	signed char numdatbytes;
//	while (UART_RX_BUFFER.NonISRbytecnt > 0)
//	{
//        if (*(UART_RX_BUFFER.NonISRPtr.itr) == SER_ALIGN)	//an initial alignment check of header
//        {
//        	if ((numdatbytes=EnoughBytes())!=-1)	//do we have enough bytes in the buffer to parse the command?
//        	{
//       			if (ValidChkSum(numdatbytes))		//we've got the correct number of bytes in buffer; does checksum pass?
//       			{
//		        	//at this point, we know we have sufficient data to parse, and we have valid checksum
//		        	unsigned char cmd=*(UART_RX_BUFFER.NonISRPtr.ind(2));//look for the cmd that is present...
//		        	unsigned char address=*(UART_RX_BUFFER.NonISRPtr.ind(1));//look for the address (channel number) that is present...
//					if (address<=NumChannels && address>0)	//we're addressing a single channel only
//						Controller[address-1].ProcessCommand(cmd, numdatbytes, true);	//Note that JRK normally uses channel nubers starting at 1; 0 is reserved for the default channel, however our channels start at 0 and are NOT programmable
//					else if (address == 0) Controller[address].SendStatus(0x00);	//we're addressing the default channel at address 0. this should only be done when the JRK system is trying to initially register, so just send a default status byte from channel 0
//					else if ((address == 0xFF) && ((cmd & 0x0F) == 0x0F))	//special case where hard reset was sent to 0xFF (the default address for resets). NOTE: this test MUST precede the regular group processing test
//					{
//						int i;
//						for (i=0;i<NumChannels;i++)	//scann all channels and
//						{	//process the command
//							Controller[i].ProcessCommand(cmd, numdatbytes, false);
//						}
//					}
//					else if (address>=0x80)		//we're addressing a group
//					{
//						int i;
//						for (i=0;i<NumChannels;i++)	//scann all channels and
//						{	//if the group address for the channel matches, process the command
//							if (Controller[i].GroupAddr==address) Controller[i].ProcessCommand(cmd, numdatbytes, false);
//						}
//					}
//					//do this last after successful parse & cmd issue
//					UART_RX_BUFFER.FlushNChars(4+numdatbytes);	//ok, successful parse. Flush the msg from the buffer
//       			}
//       			else	//invalid checksum; flush relevant bytes from buffer
//       			{
//       				UART_RX_BUFFER.FlushNChars(4+numdatbytes);
//       			}
//        	}
//        	else goto leave_parse;	//we have proper alignment, but not enough bytes to chksum or parse the current command
//        }
//        else //if the element at the NonISRPtr is not the alignment char...
//        {
//        	UART_RX_BUFFER.FlushNChars(1);	//flush one char from the buffer
//        }
//	}
//	leave_parse: _no_operation();	//this label is used by a GoTo when we have a partial command in the buffer, and are waiting for more chars. NoOp basically holds the line for the label
//
//}

///********************************************
// * EnoughBytes - checks for a sufficient number of bytes in the
// * buffer, based on the number of bytes indicated in the cmd byte.
// * If the buffer containes enough bytes for the cmd, it returns the
// * number of bytes. If there are not enough bytes, it returns -1.
// * Fn requires that the buffer is correctly aligned.
// * NOTE: the number here is the number of DATA bytes in the command,
// * as defined by the JR Kerr standard. 0 is a valid number, since a cmd
// * with 0 bytes has just the header, address, command and chksum bytes.
// *******************************************/
// signed char EnoughBytes(void)
// {
// 	//if (*(UART_RX_BUFFER.NonISRPtr.itr) != 0xAA) return -1;	//basic alignment checking first
// 	unsigned char numdatbytes=(*(UART_RX_BUFFER.NonISRPtr.ind(2)));	//get the command, with the number of bytes
// 	numdatbytes=numdatbytes>>4;	//shift over by 4 bits to get the number of bytes
// 	if (UART_RX_BUFFER.NonISRbytecnt<(numdatbytes+4)) return -1;	//there aren't enough bytes available in the buffer to parse the cmd
// 	else return numdatbytes;
// }
///*******************************************/

///********************************************
// * ValidChkSum - checks for a valid checksum. If both check out.
// * then it returns true. It is supplied with the number of
// * bytes that the particular command expects. This count includes
// * the number of DATA bytes in the command,
// * as defined by the JR Kerr standard. Note that the 0xAA frame
// * start delimiter is ignored for incoming checksum calc.
// * Assumes correct buffer alignment.
// *******************************************/
// bool ValidChkSum(int numdatbytes)
// {
// 	unsigned char chksum = *(UART_RX_BUFFER.NonISRPtr.ind(1));	//the address
// 	chksum += *(UART_RX_BUFFER.NonISRPtr.ind(2));	//the command
// 	unsigned char i;
// 	for (i=0;i<numdatbytes;i++) chksum += *(UART_RX_BUFFER.NonISRPtr.ind(3+i));
// 	if (chksum==*(UART_RX_BUFFER.NonISRPtr.ind(3+numdatbytes))) return true;
// 	else return false;
// }
///*******************************************/


 /*********************************************************
UART ISRs: this code handles USART interrupts (for serial connection to a host PC)
  *********************************************************/
#ifdef __MSP430F2274__
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void)
 {
	 if((Generic_RX_IntFlag_Reg & Generic_RX_Flag_Mask)==Generic_RX_Flag_Mask)
	 {
		 //tx_byte_to_slave(UCA0RXBUF); //TESTING !!! echo the byte out
		 *(UART_RX_BUFFER.ISRPtr.itr)=UCA0RXBUF;	//Copy Data
		 UART_RX_BUFFER.ISRPtr.inc();	//increment the pointer to the next free element in the buffer
		 UART_RX_BUFFER.ISRbytecnt++;	//increment the new data counter
	 }
 }
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
 {
	 //did we get an ISR req from the desired module?
	 if ((Generic_TX_IntFlag_Reg & Generic_TX_Flag_Mask)==Generic_TX_Flag_Mask)
	 {
		 Generic_TX_Buf  = *UART_TX_BUFFER.ISRPtr.itr;	//get the data to send
		 UART_TX_BUFFER.ISRPtr.inc();
		 UART_TX_BUFFER.ISRbytecnt--;
		 if (UART_TX_BUFFER.ISRbytecnt<=0)	//did we just send the last byte in the buffer?
		 {
			 UART_TX_BUFFER.ISRbytecnt = 0;
			 Generic_TX_IntEn_Reg &= ~(Generic_TX_IntEn_Mask);	//last byte in buffer was sent, so disable the TX interrupt
		 }
	 }
 }
#endif

#ifdef __MSP430F5438__
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
  	switch(__even_in_range(UCA0IV,4))
  	{
  		case 0:break;	// Vector 0 - no interrupt
  		case 2:    		// Vector 2 - RXIFG
     		//tx_byte_to_slave(UCA0RXBUF); //TESTING !!! echo the byte out
			*(UART_RX_BUFFER.ISRPtr.itr)=UCA0RXBUF;	//Copy Data
			UART_RX_BUFFER.ISRPtr.inc();	//increment the pointer to the next free element in the buffer
			UART_RX_BUFFER.ISRbytecnt++;	//increment the new data counter
    		break;
  		case 4:			// Vector 4 - TXIFG
			Generic_TX_Buf  = *UART_TX_BUFFER.ISRPtr.itr;	//get the data to send
			UART_TX_BUFFER.ISRPtr.inc();
			UART_TX_BUFFER.ISRbytecnt--;
			if (UART_TX_BUFFER.ISRbytecnt<=0)	//did we just send the last byte in the buffer?
			{
				UART_TX_BUFFER.ISRbytecnt = 0;
				Generic_TX_IntEn_Reg &= ~(Generic_TX_IntEn_Mask);	//last byte in buffer was sent, so disable the TX interrupt
			}
  			break;                             
  		default: break;
  	}
}
#endif
/********************************************************/
