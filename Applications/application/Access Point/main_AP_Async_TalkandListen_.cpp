

/**********************************************************************************************
* Ribosome robot wireless control
* RBM - 4/30/14
**********************************************************************************************/
#include <string.h>

//wrap all header files that are written in C with this identifier so they can be called properly as C fns
extern "C" {
#include "bsp.h"
#include "mrfi.h"
#include "bsp_leds.h"
#include "bsp_buttons.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "nwk_frame.h"
#include "nwk.h"
}

#include "UART_HANDLER.hpp"	//include the UART code

void toggleLED(uint8_t);

/**************************** COMMENTS ON ASYNC LISTEN APPLICATION ***********************
Summary:
  This AP build includes implementation of an unknown number of end device peers in
  addition to AP functionality. In this scenario all End Devices establish a link to
  the AP and only to the AP. The AP acts as a data hub. All End Device peers are on
  the AP and not on other distinct ED platforms.

  There is still a limit to the number of peers supported on the AP that is defined
  by the macro NUM_CONNECTIONS. The AP will support NUM_CONNECTIONS or fewer peers
  but the exact number does not need to be known at build time.

  In this special but common scenario SimpliciTI restricts each End Device object to a
  single connection to the AP. If multiple logical connections are required these must
  be accommodated by supporting contexts in the application payload itself.

Solution overview:
  When a new peer connection is required the AP main loop must be notified. In essence
  the main loop polls a semaphore to know whether to begin listening for a peer Link
  request from a new End Device. There are two solutions: automatic notification and
  external notification. The only difference between the automatic notification
  solution and the external notification solution is how the listen semaphore is
  set. In the external notification solution the sempahore is set by the user when the
  AP is stimulated for example by a button press or a commend over a serial link. In the
  automatic scheme the notification is accomplished as a side effect of a new End Device
  joining.

  The Rx callback must be implemented. When the callback is invoked with a non-zero
  Link ID the handler could set a semaphore that alerts the main work loop that a
  SMPL_Receive() can be executed successfully on that Link ID.

  If the callback conveys an argument (LinkID) of 0 then a new device has joined the
  network. A SMPL_LinkListen() should be executed.

  Whether the joining device supports ED objects is indirectly inferred on the joining
  device from the setting of the NUM_CONNECTIONS macro. The value of this macro should
  be non-zero only if ED objects exist on the device. This macro is always non-zero
  for ED-only devices. But Range Extenders may or may not support ED objects. The macro
  should be be set to 0 for REs that do not also support ED objects. This prevents the
  Access Point from reserving resources for a joinng device that does not support any
  End Device Objects and it prevents the AP from executing a SMPL_LinkListen(). The
  Access Point will not ever see a Link frame if the joining device does not support
  any connections.

  Each joining device must execute a SMPL_Link() after receiving the join reply from the
  Access Point. The Access Point will be listening.

 *************************** END COMMENTS ON ASYNC LISTEN APPLICATION ********************/

/************  THIS SOURCE FILE REPRESENTS THE AUTOMATIC NOTIFICATION SOLUTION ************/

#define CTRL_MSG_SZ (2+6*NUM_MOTORS)	//each motor is supplied 3 16 bit words that control the amplitude, freq, and phase of actuation. Send the lowest byte of each word first.

#define SPIN_ABOUT_A_QUARTER_SECOND   NWK_DELAY(250)
#define JAM_TIME_INTERVAL 2		//the number of seconds between time "Jam" transmissions

/**********************************
 * user fn declarations
**********************************/
// callback handler
static uint8_t sCB(linkID_t);

// received message handler
static void proc_RXRF_Msg(linkID_t, uint8_t *, uint8_t);
void ParseRXCmd(int numchars);
/*********************************/


/**********************************
 * user global variables
**********************************/
/* reserve space for the maximum possible peer Link IDs */
static linkID_t sLID[NUM_CONNECTIONS] = {0};
static uint8_t  sNumCurrentPeers = 0;

//declare the external UART buffers
extern Circ_UART_Buf UART_RX_BUFFER;
extern Circ_UART_Buf UART_TX_BUFFER;

float time = 0.0;
float next_time_Jam = time+JAM_TIME_INTERVAL;

unsigned char TX_Time_msg[6];	//the time message
unsigned char TX_msg[MAX_APP_PAYLOAD];	//reserve space for the message that will be sent via radio
unsigned char TX_msg_Len = 0;

/* work loop semaphores */
static volatile uint8_t RXPeerFrameSem = 0;
static volatile uint8_t TxPeerFrameSem = 0;
static volatile uint8_t TxTimeFrameSem = 0;
static volatile uint8_t sJoinSem = 0;

//define this device's unique address
addr_t lAddr = THIS_DEVICE_ADDRESS;

/* blink LEDs when channel changes... */
static volatile uint8_t sBlinky = 0;

/*********************************/


/**********************************
 * InitTimer
 * Setup Timer_A to output the PWM signal that we care about and to also keep our timebase for the
**********************************/
void InitTimer(void)
{
	TA0CCR0 = (PWMPeriod);	//
	TA0CCTL0 = CCIE;	// CCR0 interrupt enabled
	TA0CTL = (TASSEL_2+ID_3+MC_1+TACLR);	//SMCLK,Div=8,UP to CCR0 mode,clear counter. Timer runs at 1MHz
}
/*********************************/

/**********************************
 * Timer A0 CCR0 ISR
**********************************/
#pragma vector=TIMERA0_VECTOR
__interrupt void TIMERA0_ISR(void)
{
	time+=Timestep;

}
/*********************************/



void main (void)
{
	bspIState_t intState;

	BSP_Init();	//init the board. Sets MCLK & SMCLK to 8MHz using DCO
	Init_UART();	//setup the UART component
	InitTimer();	//start the timer counting

	//set the device address from the one declared in the *_config_XX.dat file. Must be done before call to SMPL_Init()
	SMPL_Ioctl(IOCTL_OBJ_ADDR, IOCTL_ACT_SET, &lAddr);

	SMPL_Init(sCB);	//init the radio and register callback handler. ENABLES GIE AT END

	/* green and red LEDs on solid to indicate waiting for a Join. */
	if (!BSP_LED2_IS_ON())
	{
		toggleLED(2);
	}
	if (!BSP_LED1_IS_ON())
	{
		toggleLED(1);
	}

	/* main work loop */
	while (1)
	{
		//Wait for the Join semaphore to be set by the receipt of a Join frame from a device that supports an End Device.
		if (sJoinSem && (sNumCurrentPeers < NUM_CONNECTIONS))
		{
			/* listen for a new connection */
			while (1)
			{
				if (SMPL_SUCCESS == SMPL_LinkListen(&sLID[sNumCurrentPeers]))
				{
					break;
				}
				/* Implement fail-to-link policy here. otherwise, listen again. */
			}
			sNumCurrentPeers++;
			BSP_ENTER_CRITICAL_SECTION(intState);
			sJoinSem--;
			BSP_EXIT_CRITICAL_SECTION(intState);
		}

		//is it time to send a time msg? Send same msg to all peers via broadcast
		if (time>next_time_Jam)
		{
			TX_Time_msg[0]=0xFE;
			TX_Time_msg[1]=0xFF;
			unsigned char* ind=(unsigned char*)&time;
			TX_Time_msg[2]=*(ind+0);	//dereference the pointer and use some pointer addressing to make this work
			TX_Time_msg[3]=*(ind+1);	//dereference the pointer and use some pointer addressing to make this work
			TX_Time_msg[4]=*(ind+2);	//dereference the pointer and use some pointer addressing to make this work
			TX_Time_msg[5]=*(ind+3);	//dereference the pointer and use some pointer addressing to make this work
			SMPL_Send(SMPL_LINKID_USER_UUD, TX_Time_msg, 6);
			next_time_Jam=time+JAM_TIME_INTERVAL;	//update the next time we need to send a time Jam
		}

		//is it time to send a msg? Send same msg to all peers
		if (TxPeerFrameSem)
		{
			uint8_t	i;
			for (i=0; i<sNumCurrentPeers; ++i)
			{
				if (SMPL_SUCCESS == SMPL_Send(sLID[i], TX_msg, TX_msg_Len))
				{
//					BSP_ENTER_CRITICAL_SECTION(intState);
//					TxPeerFrameSem--;
//					BSP_EXIT_CRITICAL_SECTION(intState);
				}
			}
			BSP_ENTER_CRITICAL_SECTION(intState);
			TxPeerFrameSem--;
			BSP_EXIT_CRITICAL_SECTION(intState);
			TX_msg_Len = 0;	// clear the message length after an assumed TX event
		}

		// Have we received a frame on one of the ED connections?
		if (RXPeerFrameSem)
		{
			uint8_t     msg[MAX_APP_PAYLOAD], len, i;

			/* process frames from all possible peers */
			for (i=0; i<sNumCurrentPeers; ++i)
			{
				if (SMPL_SUCCESS == SMPL_Receive(sLID[i], msg, &len))
				{
					proc_RXRF_Msg(sLID[i], msg, len);

					BSP_ENTER_CRITICAL_SECTION(intState);
					RXPeerFrameSem--;
					BSP_EXIT_CRITICAL_SECTION(intState);
				}
			}
		}

		//is enough new data available to do something with?
		int numcharsavail = UART_RX_BUFFER.DataAvail();
		if(numcharsavail >= CTRL_MSG_SZ)
		{
			ParseRXCmd(CTRL_MSG_SZ);	//if we have enough bytes in the buffer parse it...
	 	 	UART_RX_BUFFER.FlushNChars(CTRL_MSG_SZ);	//...and now flush the buffer of the data we just did something with
		}
	}
}

void toggleLED(uint8_t which)
{
	if (1 == which)
	{
		BSP_TOGGLE_LED1();
	}
	else if (2 == which)
	{
		BSP_TOGGLE_LED2();
	}
	return;
}

/* Runs in ISR context. Reading the frame should be done in the */
/* application thread not in the ISR thread. */
static uint8_t sCB(linkID_t lid)
{
	//did we get a data message from a peer?
	if (lid)
	{
		RXPeerFrameSem++;
	}
	//we got a join msg
	else
	{
		sJoinSem++;
	}

	/* leave frame to be read by application. */
	return 0;
}

static void proc_RXRF_Msg(linkID_t lid, uint8_t *msg, uint8_t len)
{
	/* do something useful */
	if (len)
	{
		toggleLED(1);
		tx_bytes_to_slave(msg, len); // print ED message to UART
	}
	return;
}


void ParseRXCmd(int numchars)
{
	//copy the message to send (also echo the msg)
	for (int i=0; i<numchars; ++i)
	{
		TX_msg[i]=*(UART_RX_BUFFER.NonISRPtr.ind(i));
	}
	tx_bytes_to_slave(TX_msg, numchars);	//echo out the message
	TX_msg_Len = numchars;
	TxPeerFrameSem++; //notify that there are data to send via radio
}



////TESTING ISRS TO TRAP AFTER A RESET
//#pragma vector=PORT1_VECTOR
//__interrupt void PORT1_TST(void)
//{
//	_nop();
//}
//
////#pragma vector=PORT2_VECTOR
////__interrupt void PORT2_TST(void)
////{
////	_nop();
////}
//
//#pragma vector=ADC10_VECTOR
//__interrupt void ADC10_TST(void)
//{
//	_nop();
//}
//
//#pragma vector=USCIAB0TX_VECTOR
//__interrupt void USCIAB0TX_TST(void)
//{
//	_nop();
//}
//
//#pragma vector=USCIAB0RX_VECTOR
//__interrupt void USCIAB0RX_TST(void)
//{
//	_nop();
//}
//
//#pragma vector=TIMERA1_VECTOR
//__interrupt void TIMERA1_TST(void)
//{
//	_nop();
//}
//
//#pragma vector=TIMERA0_VECTOR
//__interrupt void TIMERA0_TST(void)
//{
//	_nop();
//}
//
//#pragma vector=WDT_VECTOR
//__interrupt void WDT_TST(void)
//{
//	_nop();
//}
//
//#pragma vector=TIMERB1_VECTOR
//__interrupt void TIMERB1_TST(void)
//{
//	_nop();
//}
//
//#pragma vector=TIMERB0_VECTOR
//__interrupt void TIMERB0_TST(void)
//{
//	_nop();
//}
//
//#pragma vector=NMI_VECTOR
//__interrupt void NMI_TST(void)
//{
//	_nop();
//}
//
////#pragma vector=RESET_VECTOR
////__interrupt void RESET_TST(void)
////{
////	_nop();
////}



