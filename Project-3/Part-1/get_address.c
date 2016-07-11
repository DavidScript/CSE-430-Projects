//CSE 430: Project 3 - Memory Management
//Date: 4/1/2016

//David McKay

//Headers:
#include <stdio.h>
#define __NR_my_syscall 359

//Data:
typedef struct{
    int pid;
	char tty[64];
	int hour;
	int min;
	int sec;
	char cmd[64];
	int result; //0 - invalid page | 1 - physical address | 2 - swap address
	unsigned long pfn;
	unsigned long long swap;
} myProcessInfo;

//Main: Project 3 - Find Physical/Swap Memory for the Process with PID.
int main(int argc, char **argv)
{
	//Checking that the User Gives PID and Virtual Memory Address
	if(argc < 3) {
		printf("Usage: get_address PID Address\n");
		return 0;
	}

	//Create Userspace Buffer:
	char myBuffer[1500];
	int i = 0;
	int sizeArray = 0;
	int myPID = strtol(argv[1], NULL, 10);
	unsigned long myAddress = strtoul(argv[2], NULL, 16);

	//Call the System Call:
	sizeArray = syscall(__NR_my_syscall, sizeArray, sizeof(myBuffer), myBuffer, myPID, myAddress);

	//Casting the Buffer into myProcessInfo Struct*
	myProcessInfo* myPI = (myProcessInfo*) myBuffer;

	//Printing out the Invalid Page:
	if(myPI[i].result == 0){
		printf("   page is not available\n");

	//Printing out the  Physical Address:
	}else if(myPI[i].result == 1){
		printf("\tphysical:\n\t\t[PFN]: %lu\n\t\t[Physical Address]: 0x%08x\n",
			myPI[i].pfn,
			(int)((myPI[i].pfn << 12) | (myAddress & 0x00000FFF)));

	//Printing out the Swap Address:
	}else{
		printf("\tswap:\n\t\t[Swap ID]: %llu\n\t\t[Swap Offset]: %llu\n",
			(myPI[i].swap & 0x7FFFFFFFFFFFFF) << 5,
			(myPI[i].swap & 0x7FFFFFFFFFFFFF));

	}

	//Exit:
	return(0);
}
