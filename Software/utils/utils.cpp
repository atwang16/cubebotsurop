/******************************************************
 * Copyright 2011 - Robert MacCurdy. All rights reserved
******************************************************/

/**************************************************
 * This file includes small helper fns that I've 
 * developed over the years that make life easier
**************************************************/


#include "utils.hpp"

/******************************************************
 * variable declarations & initializations
 ******************************************************/
#ifdef use_Robs_Presets
//Data backup objects. IMPORTANT: this object MUST be declared prior to any objects that might want to backup their variables with the "RegisterBackup" function
#pragma DATA_SECTION(".Backup")	//places the NEXT declared object (InfoFlash) into info flash memory
unsigned char InfoFlash[NumNVBytes];	//declare non-volatile storage space in the info flash where the device presets will be stored at shutdown. IMPORTANT: this object needs spans several Info flash segments - be aware of this. 
Presets Presets_Store(InfoFlash);	//the presets storage object
#endif

/**************************************************
 * Disable_Int & Restore_Int - these fns disable interrupts, 
 * but first make a copy of the state of the GIE bit
 * which can be used later in Restore_Int to return
 * to whatever state we were in before. This is useful
 * when code needs to ensure interrupts won't happen,
 * but it is potentially risky to actually enable 
 * interrupts, since they might not have been enabed
 * in the first place.
***************************************************/
unsigned short Disable_Int(void)
{
	unsigned short GIE_state = GIE & _get_interrupt_state();	//get the status of the global interrupt enable bit
	_disable_interrupts();	// 5xx Workaround: Disable global interrupt while erasing. Re-Enable GIE if needed
	return GIE_state;
}
void Restore_Int(unsigned short GIE_state)
{
	_bis_SR_register(GIE_state);	//logically ORs the SR with GIE_state, so it sets GIE if it was set earlier, and preserves the rest of the register (since we masked GIE_state off earlier)
}
/**************************************************/

/**************************************************
 * GenChkSum - passed the pointer to an array of unsigned
 * chars and the number of entries, the fn returns an 
 * unsigned char with the summation modulo-8 bits.
***************************************************/
unsigned char GenChkSum(unsigned char * arry, int numele)
{
	int i;
	unsigned char chksum=0;
	for (i=0;i<numele;i++) chksum+=*(arry+i);
	return chksum;
}
/**************************************************/	

/**************************************************
 * _evenPar - Checks if a bit sequence has even parity
***************************************************/
//these two circular shift routines assume that the operand is a 16 bit unsigned type
unsigned int _rotl16(unsigned int value, int shift) {
    if ((shift &= 15) == 0)
      return value;
    return (value << shift) | (value >> (16 - shift));
}
unsigned int _rotr16(unsigned int value, int shift) {
    if ((shift &= 15) == 0)
      return value;
    return (value >> shift) | (value << (16 - shift));
}
//fn returns true if the even parity bit returned from the encoder matches the parity of the rotary encoder's position data word
unsigned int _evenPar(unsigned int value, unsigned char par){
	unsigned int tmp=_rotl16(value,1);
	value=value^tmp;
	tmp=_rotl16(value,2);
	value=value^tmp;
	tmp=_rotl16(value,4);
	value=value^tmp;
	tmp=_rotl16(value,8);
	value=~(value^tmp);	//invert the answer. at this point, every bit represents the even parity: high if the number of 1's in the input was even	
	return !((!(value & 0x0001))^(!(0x20 & par)));	//return true if the sent parity bit matches the computed parity
}
/**************************************************/
	
#ifdef use_Robs_Presets
/*********************************************************
ISR: SYSNMI_ISR - this code handles SYSNMI interrupts (for saving
 * RAM to info FLASH on power-off). Uses SVM High triggers to detect 
 * a falling supply voltage and saves to FLASH before a BOR event
 * occurs. The interrupt we want is enabled by setting SVMHIE in the PMM module
*********************************************************/
#pragma vector=SYSNMI_VECTOR
__interrupt void SYSNMI_ISR(void)
{
  	switch(__even_in_range(SYSSNIV,0x14))
  	{
  		case 0x00:break;	// Vector 0 - no interrupt
  		case 0x02:break;	// SVMLIFG interrupt pending (highest priority)
  		case 0x04:			// SVMHIFG interrupt pending (the one we'll use)
  			//P9OUT |= BIT7;//TESTING ONLY
  			PMMCTL0_H = 0xA5;	// Open PMM registers for write access
  			PMMRIE &=~SVMHIE;	//(possibly redundant) clear SVMHIE to avoid any potential for repeated interrupts after we've serviced this one (only want to shut down once)!
  			PMMCTL0_H = 0x00;	// Lock PMM registers for write access
  			Presets_Store.Write_Presets();		//save the presets to info flash
  			//P9OUT &= ~BIT7;//TESTING ONLY
  			_no_operation();	//buffer a few in here in case of problems
  			_no_operation();
  			_no_operation();
  			_delay_cycles(0xFFFFFFFF);	//power is going down, so go to sleep gracefully
  			_low_power_mode_4();	//power is going down, so go to sleep gracefully (not sure which of these two alternatives to choose. keeping CPU running might make it use more pwer and reset faster)
  			break;
  		case 0x06:break;	// SVSMLDLYIFG interrupt pending
  		case 0x08:break;	// SVSMHDLYIFG interrupt pending
  		case 0x0A:break;	// VMAIFG interrupt pending
  		case 0x0C:break;	// JMBINIFG interrupt pending
  		case 0x0E:break;	// JMBOUTIFG interrupt pending
  		case 0x10:break;	// SVMLVLRIFG interrupt pending
  		case 0x12:break;	// SVMHVLRIFG interrupt pending
  		case 0x14:break;	// Reserved for future extensions
  		default: break;
  	}	
}
/********************************************************/
#endif


// UNUSED FOR NOW
////Class encapsulates fractional numbers and arithmetic
//class QDot
//{
//	int op1;
//	#define FracBits	15	//this number is the number of bits in the word that represent the fraction (MSb is sign)
//	#define K   (1 << (FracBits-1))
//	public:
//	
//	QDot(int op2){
//		op1=op2;
//	}
//	
//	int operator*(QDot op2)
//	{
//		int result;
//		long int  temp;
//		temp = (long int) op1 * (long int) op2.op1; // result type is operand's type
//		// Rounding; mid values are rounded up
//		temp += K;
//		// Correct by dividing by base
//		result = temp >> FracBits;
//		return result;		
//	}
//	void operator=(int op2)
//	{
//		op1=op2;		
//	}
//	int operator+(QDot op2)
//	{
//		int result=op1+op2.op1;
//		return 	result;	
//	}
//};


