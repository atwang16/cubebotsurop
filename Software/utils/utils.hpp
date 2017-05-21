/******************************************************
 * Copyright 2011 - Robert MacCurdy. All rights reserved
******************************************************/

#ifndef UTILS_H_
#define UTILS_H_

#ifdef __MSP430F2274__
#include <msp430f2274.h>  //add in the device that we'll use here
#endif

#ifdef __MSP430F5438__
#include <msp430f5438.h>  //add in the device that we'll use here
#endif

/**************************************************
 * This file includes small helper fns and classes that I've 
 * developed over the years that make life easier
**************************************************/

/**********************************************
 * A couple of interrupt enable/disable fns that preserve the prev ISR state
**********************************************/
unsigned short Disable_Int(void);
void Restore_Int(unsigned short GIE_state);
/*********************************************/

/**********************************************
 * simple byte-wide checksum generator
**********************************************/
unsigned char GenChkSum(unsigned char * arry, int numele);
/*********************************************/

/**************************************************
 * _evenPar - Checks if a bit sequence has even parity
***************************************************/
unsigned int _evenPar(unsigned int value, unsigned char par);
/**************************************************/

/**********************************************
 * A simple iterator class for use within circular buffers.
 * The constructor requires the starting address of the circular buffer, 
 * and the number of Entries in the buffer. The type of the buffer is taken
 * care of where the buffer is statically created.
**********************************************/
template <class datatype> class iterator
{
	datatype *start;	//starting address of the circular buffer
	datatype *end;		//end address of the circular buffer
	public:
	datatype *itr;		//the iterator object
	
	iterator():start(),end(),itr(){}

	//initialize the iterator; MUST be called.
	void init(datatype *s, int L)
	{
		start=s;
		end=s+(L-1);
		itr=s;
	}
	//simple iterator increment
	void inc(void)
	{
		itr++;	//increment by one entry
		if (itr>end) itr=start;	//handle over-run of buffer
	}
	//simple iterator decrement
	void dec(void)
	{
		itr--;
		if (itr<end) itr=end;	//handle over-run of buffer
	}
	//used to index forward from the iterator WITHOUT CHANGING the iterator. op2 MUST BE POSITIVE
	datatype * ind(int op2)
	{
		datatype * tmp=itr;
		tmp+=op2;
		if (tmp>end) tmp=tmp-end-1+start;
		return tmp;
	}
	//used to advance iterator forward op2 steps. op2 MUST BE POSITIVE
	void adv(int op2)
	{
		itr+=op2;
		if (itr>end) itr=itr-end-1+start;
	}
};
/*********************************************/

#ifdef use_Robs_FxdPt
/**********************************************
 * FxdPt - Fixed Pt Class allows computation with 32 bit
 * signed integers, where the lower 15 bits are fractional and
 * the upper 16 bits are integer. MSb is sign, as these are stored
 * in normal two's complement style. Note that the lowest bit is
 * a throwaway, since the fractional component also contained a (redundant)
 * sign bit. This uses the HW Multiplier
 * for computational efficiency. The HWM routines must be included by adding the "fixed_pt.asm" file to the project directory.
**********************************************/
extern "C" bool _HWM_FxdPtXFxdPt(long * op1, long * op2);	//the functions that use the HWM to do my fixed point multiplications. See file: "fixed_pt.asm". Note the "C" after extern tells the Compiler not to mangle the object name, so we can easily use the same name in the .asm file
extern "C" bool _HWM_FxdPtXLong(long * op1, long * op2);
class FxdPt
{
	private:
	long data;	//16 bits of integer and 15 bits of fractional data. Divide this number by 10000h for the real number it represents.
	bool OvFlw;	//flag indicates if an overflow event has ever occured for this object
	
	public:
	FxdPt(){data=0;OvFlw=false;}
	
	//simple assignment operators
	void operator = (long op1) {data=op1;} //direct assignment by properly formatted number (multiply the desired real # by 0x8000h to get the formatted fixed pt number)
	void operator = (FxdPt op1) {data=op1.data;}	//assignment from another FxdPt object
	
	//full assignment to load the data type with integer and fractional data
	//op1 is the SIGNED integer portion of the data (sign of op1 and op2 should match unless op1 is 0); op2 is the SIGNED fractional portion.
	//calculate op2 by multiplying your desired fractional number by 0x8000h (be sure to round down so that .9999999*0x8000h=0x7FFFh, since 0x8000h=-1)
	void SetData(int op1, int op2)
	{
		int * tmp = (int *) &data;
		long tmp2 = ((long)op2)<<1;	//sign extend the fractional data and align for proper addition later
		data=0;
		*(tmp+1) = op1;	//move the signed integer data
		data+=tmp2;		//add in the fractional portion
	}
	
	long GetData(){ return data;}	//get the raw 32 bit data
	
	int GetInteger()	//get the integer portion of the fixed point data
	{
		int * tmp = (int *) &data;
		return *(tmp+1);
	}
		
	//PLEASE NOTE: in order to handle overflow checking at each stage, the first operator for these
	//fns is passed by ref, and the result is returned by ref, so an assignment operator 
	//on the output of these fns (like A=A*B) is not needed, and will just result in inefficient copies.
	//The result is returned in the object that the member operated on (this).
	//Returns true if no pos/neg overflow.
	//Clamps to max pos/neg value if overflow and returns false. No underflow to zero checking!
	//Uses HWM for operation via assembled code in "fixed_pt.asm".
	FxdPt operator * (FxdPt op1)
	{
		FxdPt * tmp=this;
		if (_HWM_FxdPtXFxdPt(&data, &op1.data) && OvFlw==false) OvFlw=false;	//if we didn't overflow and we weren't already overflowed, then signal no overflow
		else OvFlw=true;
		return *tmp;
	}
	
	//multiply one of these by an integer. Result is in *this. Returns true if no pos/neg overflow
	//Clamps to max/min value if overflow. Uses HWM for operation, but accesses result registers
	//directly; 
	//remember, result needs to fit into 16 bit integer and 16 bit fractional space, so don't overflow this
	FxdPt operator * (long op1)
	{
		FxdPt * tmp=this;
		if (_HWM_FxdPtXLong(&data, &op1) && OvFlw==false) OvFlw=false; //if we didn't overflow and we weren't already overflowed, then signal no overflow
		else OvFlw=true;
		return *tmp;	
	}
	
	FxdPt operator + (FxdPt op1)
	{
		FxdPt * tmp=this;
		data = data + op1.data;	//should really have some assembly code here that does a fast addition, checks the carry bit for overflow, and clamps as desired
		return *tmp; 
	}

	FxdPt operator + (long op1)
	{
		FxdPt * tmp=this;
		data = data + op1;//should really have some assembly code here that does a fast addition, checks the carry bit for overflow, and clamps as desired
		return *tmp; 
	}
	
	FxdPt operator - (FxdPt op1)
	{
		FxdPt * tmp=this;
		data = data - op1.data;//should really have some assembly code here that does a fast addition, checks the carry bit for overflow, and clamps as desired
		return *tmp; 
	}

	FxdPt operator - (long op1)
	{
		FxdPt * tmp=this;
		data = data - op1;//should really have some assembly code here that does a fast addition, checks the carry bit for overflow, and clamps as desired
		return *tmp; 
	}	
};
/*********************************************/
#endif

#ifdef use_Robs_Presets
/******************************************************
 * Presets - this class basically just provides storage 
 * for the class data from other classes that we want to
 * backup into Info memory. Each block of Info memory
 * has 128 bytes, and Info memory is contiguous (at least 
 * in the MSP5438), and grows upward in the memory space 
 * as new memory is allocated. As a convention in the code
 * I indicate variables that will be backed up with "(BU)"
 * and those that will not be backed up with "(NBU)"
******************************************************/
class Presets
{
#define NumNVBytes	384		//number of bytes that will be allocated in Info Flash for system parameter storage

	class Preset_Entry
	{
		public:
		unsigned char * entryptr;	//holds the starting address of the data
		unsigned char	numbytes;	//stores the number of bytes that the data contains
	};
	
	Preset_Entry Preset_Entries[NumNVBytes];	//RAM storage for all the pointers to memory that will be backed up
	unsigned char *NonVolStor;			//pointer to the first byte of storage allocated in NonVolatile Flash
	unsigned int NonVolStor_EntryCnt;	//number of data entries actually stored in NV Flash. NOTE: to prevent unallocated pointers, this is updated each time a pointer is assigned by "RegisterBackup" to "Preset_Entries.entryptr"
	unsigned char dat_Valid;		//this memory address just holds a key that allows us to check if the flash has previously been loaded with valid data. It's used to identify if the flash memory has been written or is blank. Recall that erased Flash memory reads: "FF"
	unsigned int Preset_Entries_Ind;
	
	public:
	Presets(unsigned char *NVStor){
		NonVolStor = NVStor;
		NonVolStor_EntryCnt = 1;	//stores the number of valid entries that exist in Preset_Entries
		dat_Valid=0x0A;		//this memory address just holds a key that allows us to check if the flash has previously been loaded with valid data
		Preset_Entries[0].entryptr = &dat_Valid;
		Preset_Entries[0].numbytes = 1;
	}
	
	
	/********************************************************
	 * Read_Presets(void) - Uses flash memory to recall
	 * system preset data. Note that the relationship between Flash locations and
	 * RAM locations is controlled by the order that "RegisterBackup()" is called by constructors 
	 * of modules that want to have non-volatile data backup
	********************************************************/
	void Read_Presets(void)
	{
		unsigned int i,j;
		unsigned char *PtrToinfoFlash=NonVolStor;
		unsigned char *PtrToRAM;
		if (*PtrToinfoFlash==0x0A){ //does the memory contain initialized data?			
			for (i=0; i<NonVolStor_EntryCnt; i++){
				PtrToRAM=Preset_Entries[i].entryptr;	//get the starting address of the RAM location that we want to restore
				for (j=0; j<Preset_Entries[i].numbytes; j++){				
				 	*PtrToRAM++=*PtrToinfoFlash++;	// Write flash value to RAM address location
				}
			}
		}
	}
	
	/********************************************************
	 * Write_Presets(void) - similar to Read_Presets
	********************************************************/	
	void Write_Presets(void)
	{
		unsigned int i,j;
		unsigned char *PtrToinfoFlash=NonVolStor;
		unsigned char *PtrToRAM;		
		unsigned short GIE_state = Disable_Int();
		FCTL3 = FWKEY;        	// Clear Lock bit
		FCTL1 = FWKEY+ERASE;   	// Set Erase bit
		*PtrToinfoFlash = 0;       	// Dummy write to erase Flash seg
		*(PtrToinfoFlash+128) = 0;	// Dummy write to erase next Flash segment
		*(PtrToinfoFlash+256) = 0;	// Dummy write to erase next Flash segment
		FCTL1 = FWKEY+WRT;    	// Set WRT bit for write operation
		for (i=0; i<NonVolStor_EntryCnt; i++){
			PtrToRAM=Preset_Entries[i].entryptr;	//get the starting address of the RAM location that we want to restore
			for (j=0; j<Preset_Entries[i].numbytes; j++){				
			 	*PtrToinfoFlash++ = *PtrToRAM++;	// Write value from RAM to flash
			}
		}			
		FCTL1 = FWKEY;         	// Clear WRT bit
		FCTL3 = FWKEY+LOCK;    	// Set LOCK bit
		Restore_Int(GIE_state);	
	}
	
	/********************************************************
	 * RegisterBackup - fn takes the starting address of a STATICLY declared
	 * piece of data, and registers its address into the "Preset_Entries"
	 * object so that the data will be backed up at power-down
	 * The data must be statically declared (NOT on the heap/stack)
	 * so that it can be unambigously addressed by the location passed in DatAddr
	********************************************************/
	void RegisterBackup(void * DatAddr, unsigned char numbytes)
	{
		Preset_Entries[NonVolStor_EntryCnt].entryptr = (unsigned char *)DatAddr;	//store the starting address of the RAM data to be backed up
		Preset_Entries[NonVolStor_EntryCnt].numbytes = numbytes;//tell the system how many bytes of data we're backing up
		NonVolStor_EntryCnt++;
	}
	
};
/*****************************************************/
#endif


#endif /*UTILS_H_*/
