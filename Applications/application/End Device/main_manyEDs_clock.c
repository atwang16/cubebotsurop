/*----------------------------------------------------------------------------
 *  Demo Application for SimpliciTI
 *
 *  L. Friedman
 *  Texas Instruments, Inc.
 *---------------------------------------------------------------------------- */

/**********************************************************************************************
  Copyright 2007-2008 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights granted under
  the terms of a software license agreement between the user who downloaded the software,
  his/her employer (which must be your employer) and Texas Instruments Incorporated (the
  "License"). You may not use this Software unless you agree to abide by the terms of the
  License. The License limits your use, and you acknowledge, that the Software may not be
  modified, copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio frequency
  transceiver, which is integrated into your product. Other than for the foregoing purpose,
  you may not use, reproduce, copy, prepare derivative works of, modify, distribute,
  perform, display or sell this Software and/or its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS”
  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY
  WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
  IN NO EVENT SHALL TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE
  THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY
  INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST
  DATA, COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY
  THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
 **************************************************************************************************/

//includes for the radio
#include "bsp.h"
#include "mrfi.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "bsp_leds.h"
#include "bsp_buttons.h"
#include "nwk.h"

//user includes
#include "math.h"

/* work loop semaphores */
static volatile uint8_t TxPeerFrameSem;
static volatile uint8_t RxPeerFrameSem;
static volatile uint8_t RxBroadcastSem;

static void linkTo(void);

void toggleLED(uint8_t);

/* Callback handler */
static uint8_t sCB(linkID_t);

/* received message handler */
static void processMessage(linkID_t, uint8_t *, uint8_t);

static	linkID_t sLinkID1 = 0;

//define this device's unique address
//REMEMBER TO SET THE DEVICE ADDRESS TO WHAT YOU WANT IN THE "smpl_config_ED.dat" file!!! And to do a CLEAN and then Build, otherwise the address won't be properly compiled
addr_t lAddr = THIS_DEVICE_ADDRESS;

#define SPIN_ABOUT_A_SECOND   NWK_DELAY(1000)
#define SPIN_ABOUT_A_QUARTER_SECOND   NWK_DELAY(250)
#define CHANGE_DEFAULT_ROM_DEVICE_ADDRESS

/**********************************
 * user global variables
**********************************/
float amplitude = 0.0;
float frequency = 0.0;
float phase = 0.0;
float time = 0.0;
float clock_check = -1.0;
float clock_check2 = -1.0;
float init_clock_check = 0.0;
float time_correction = 0.0;
float drift_rate = 0.0;
float TIME_CORR_THRESHOLD = 0.001;
float DURATION = 5.0;
float start_time = 0.0;
#pragma DATA_ALIGN (RadioMSG, sizeof(int));	//align to int boundary so I can do nice pointer casts to get the data that I want
uint8_t     RadioMSG[MAX_APP_PAYLOAD];
/*********************************/

/**********************************
 * InitTimer
 * Setup Timer_A to output the PWM signal that we care about and to also keep our timebase for the
**********************************/
void InitTimer(void)
{
	P2DIR |= BIT3;                            // P2.3 is output
	P2SEL |= BIT3;                            // P2.3 is TACCR1 output
	TA0CCR0 = (PWMPeriod);	//
	TA0CCTL0 = CCIE;	// CCR0 interrupt enabled
	TA0CCR1 = PWMPeriod;		// CCR is not initialized
	TA0CCTL1 = (OUTMOD_3);	//set when we count to TACCR1, reset when we count to TACCR0
	TA0CTL = (TASSEL_2+ID_3+MC_1+TACLR);	//SMCLK,Div=8,UP to CCR0 mode,clear counter. Timer runs at 1MHz
}

/**********************************
 * Timer A0 CCR0 ISR
**********************************/
#pragma vector=TIMERA0_VECTOR
__interrupt void TIMERA0_ISR(void)
{
	time+=Timestep;
	if(clock_check >= 0.0) {
//	    drift_rate = (time - clock_check) / time;
	    clock_check += Timestep;
	    clock_check2 += Timestep;
	    time_correction = (time - clock_check) * 2.5;
	    if(abs(time_correction) > TIME_CORR_THRESHOLD) {
	        time_correction = 1000000 * TIME_CORR_THRESHOLD * ((time_correction > 0) ? 1 : -1);
	    }
	    else {
	        time_correction = 1000000 * time_correction;
	    }
//	    TA0CCR0 = PWMPeriod - drift_rate * PWMPeriod * 1000000;
	    TA0CCR0 = PWMPeriod - time_correction;
	    clock_check = time;
	}
	else {
	    TA0CCR0 = PWMPeriod;
	}
	//turn on the PWM generation is we're asking for a signal
	if (amplitude > 0) TA0CCR1=(PWMPeriod-1500)+(int)(500*amplitude*sinf(frequency*(time-start_time)+phase));
	else TA0CCR1=0;
}
/*********************************/


void main (void)
{
	uint8_t    done = 0;
	bspIState_t intState;
	unsigned char test_msg[9];
	unsigned char clock_msg[4];

	RxBroadcastSem=0;
	RxPeerFrameSem=0;

	BSP_Init();	//init the board
	InitTimer();	//setup the PWM generating timer

	//set the device address from the one declared in the *_config_XX.dat file. Must be done before call to SMPL_Init()
#ifdef CHANGE_DEFAULT_ROM_DEVICE_ADDRESS
	{
		//createRandomAddress(&lAddr);
		SMPL_Ioctl(IOCTL_OBJ_ADDR, IOCTL_ACT_SET, &lAddr);
	}
#endif

	//init radio, register callback and try to join. Enables GIE at end
	while (SMPL_SUCCESS != SMPL_Init(sCB))
	{
		toggleLED(1);	//toggle LEDs during Join attempt
		toggleLED(2);
		SPIN_ABOUT_A_SECOND;
	}

	/* LEDs on solid to indicate successful join. */
	if (!BSP_LED2_IS_ON())
	{
		toggleLED(2);
	}
	if (!BSP_LED1_IS_ON())
	{
		toggleLED(1);
	}

	/* Unconditional link to AP which is listening due to successful join. */
	linkTo();

	while (1)
	{

		// Have we received a broadcast msg?
		if (RxBroadcastSem)
		{
			uint8_t	len;
			if (SMPL_SUCCESS == SMPL_Receive(SMPL_LINKID_USER_UUD, RadioMSG, &len))
			{
				processMessage(SMPL_LINKID_USER_UUD, RadioMSG, len);
				BSP_ENTER_CRITICAL_SECTION(intState);
				RxBroadcastSem--;
				BSP_EXIT_CRITICAL_SECTION(intState);
			}

		}

		//is it time to send a msg?
		if (TxPeerFrameSem)
		{
			/* Set TID and designate which LED to toggle */
			done = 0;
			while (!done)
			{
				if (SMPL_SUCCESS == SMPL_Send(sLinkID1, test_msg, sizeof(test_msg)))
				{
					//Just break out since there will be no ack.
					BSP_ENTER_CRITICAL_SECTION(intState);
					TxPeerFrameSem--;
					BSP_EXIT_CRITICAL_SECTION(intState);
					break;
				}
				done = 1;
			}
		}

		// Have we received a msg?
		if (RxPeerFrameSem)
		{
			uint8_t	len;
			if (SMPL_SUCCESS == SMPL_Receive(sLinkID1, RadioMSG, &len))
			{
				processMessage(sLinkID1, RadioMSG, len);
				BSP_ENTER_CRITICAL_SECTION(intState);
				RxPeerFrameSem--;
				BSP_EXIT_CRITICAL_SECTION(intState);
			}

		}

//		if (clock_check > DURATION + init_clock_check) {
//		    toggleLED(2);
//      }

		if (time > DURATION + init_clock_check) {
		    toggleLED(1);
//		    float clock_counter = clock_check - init_clock_check;
		    float clock_counter = clock_check2 - time;
//		    float time_counter = time - init_clock_check;
		    unsigned char* clock_ind =(unsigned char*)&clock_counter;
//		    unsigned char* time_ind  =(unsigned char*)&time_counter;
		    clock_msg[0]=*(clock_ind+3);    //dereference the pointer and use some pointer addressing to make this work
            clock_msg[1]=*(clock_ind+2);    //dereference the pointer and use some pointer addressing to make this work
            clock_msg[2]=*(clock_ind+1);    //dereference the pointer and use some pointer addressing to make this work
            clock_msg[3]=*(clock_ind+0);    //dereference the pointer and use some pointer addressing to make this work
            SMPL_Send(sLinkID1, clock_msg, sizeof(clock_msg));
//            clock_msg[0]=*(time_ind+3);
//            clock_msg[1]=*(time_ind+2);
//            clock_msg[2]=*(time_ind+1);
//            clock_msg[3]=*(time_ind+0);
//		    SMPL_Send(sLinkID1, clock_msg, sizeof(clock_msg));
            DURATION += 5.0;
        }
	}
}


static void linkTo()
{

	/* Keep trying to link... */
	while (SMPL_SUCCESS != SMPL_Link(&sLinkID1))
	{
		toggleLED(1);
		toggleLED(2);
		SPIN_ABOUT_A_SECOND;
	}

	/* Turn off LEDs after successful Link */
	if (BSP_LED2_IS_ON())
	{
		toggleLED(2);
	}
	if (BSP_LED1_IS_ON())
	{
		toggleLED(1);
	}

	//for now just turn the radio on
	SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_RXON, 0);

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

static uint8_t sCB(linkID_t lid)
{
	//broadcast msg
	if (lid == SMPL_LINKID_USER_UUD)
	{
		RxBroadcastSem++;
		return 0;
	}
	//msg addressed only to this device
	else if (lid == sLinkID1)
	{
		RxPeerFrameSem++;
		return 0;
	}

	return 1;
}

/***********************************************************
 * processMessage
 * Receive an aligned and formatted data message and decode it
 * Each motor gets 3 ints of data, amplitude, frequency and phase
 * that are packed in a byte array, little byte first as usual
***********************************************************/
static void processMessage(linkID_t lid, uint8_t *msg, uint8_t len)
{
	int tempAmp, tempFreq, tempPhase;
	float tempAmpF, tempFreqF, tempPhaseF;
	bspIState_t intState;
	unsigned int flag=0;
	//parse the received message and set the PWM numbers accordingly
	flag=*(unsigned int*)(msg+0);
	//be sure the inital message alignment is what we expect, that it matches an expected message type, and it's only 6 bytes
	if ((flag == TIME_MSG) && (lid == SMPL_LINKID_USER_UUD) && len == 6)
	{
		BSP_ENTER_CRITICAL_SECTION(intState);	//protect from possible interrupts until we've written all values that might be used by the ISR
		time = *(float*)(msg+2);	//jam the time parameter that we just received from the radio
		if(clock_check < 0) {
//		    toggleLED(1);
		    init_clock_check = clock_check = clock_check2 = time;
		}
		BSP_EXIT_CRITICAL_SECTION(intState);
	}
	else if (flag == MOT_MSG)
	{
		int offset=6*(lAddr.addr[0]-1)+2;	//use this as an index into the received byte array. the 6 corresponds to 6 bytes of data per motor and the 2 accounds for the 2 bytes of flag data
		tempAmp=*(unsigned int*)(msg+offset);
		tempAmpF=((float)tempAmp)/500;	//convert to float and rescale since we prescaled to send as an int
		tempFreq=*(unsigned int*)(msg+offset+2);
		tempFreqF=((float)tempFreq)/500;
		tempPhase=*(unsigned int*)(msg+offset+4);
		tempPhaseF=((float)tempPhase)/500;

		//finally, copy to the system-level values and reset the time
		BSP_ENTER_CRITICAL_SECTION(intState);	//protect from possible interrupts until we've written all values that might be used by the ISR
		amplitude = tempAmpF;
		frequency = tempFreqF;
		phase = tempPhaseF;
		start_time = time;	//update the time at which we start the sinusoid so we have unambiguous phase
		BSP_EXIT_CRITICAL_SECTION(intState);
	}

	return;
}

