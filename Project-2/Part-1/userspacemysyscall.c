//CSE 430: Project 2 - Listing Processes
//Date: 2/14/2016

//David McKay

//Headers:
#include <stdio.h>
#define __NR_my_syscall 359

//Main: Project 2 - List Processes similar to ps -e
int main(void)
{
	//Create Userspace Buffer:
	char myBuffer[45000];

	//Call the System Call:
	syscall(__NR_my_syscall, 0, sizeof(myBuffer), myBuffer);

	//Example Format:
	//__PID_TTY__________TIME_CMD_
	//_1232_tty2_____00:00:00_bash_

	//Pritning out the System Call:
	printf("  PID TTY          TIME CMD \n");
	printf("%s \n", myBuffer);

	//Exit:
	return(0);
}
