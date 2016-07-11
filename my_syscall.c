//CSE 430: Project 2 - List Processes
//Date: 2/14/2016

//David McKay

//Headers:
#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/tty.h>

//Project 2: List Processes
//int a: initial starting point 0
//int b: sizeof(UserSpace_Buffer)
//char* c: Userspace_Buffer
asmlinkage long sys_my_syscall(int a, int b, char *c)
{
	//Initializing Variables:
	char kernel_buffer[50000];
	int length = 0;
	struct task_struct *task;
	unsigned long jiff;
	cputime_t utime;
	cputime_t stime;
	int n = 0;
	char timeBuffer[9];
	int hour = 0;
	int min = 0;
	int sec = 0;

	//Gathering all the Processes Information:
	for_each_process(task){

		//Calculating the Time for the Task:
		thread_group_cputime_adjusted(task, &utime, &stime);
		jiff = utime + stime;
		jiff = jiff/1000;
		hour = jiff/3600;
		min = (jiff-(hour*3600))/60;
		sec = (jiff-(hour*3600)-(min*60));
		sprintf(timeBuffer, "%.2d:%.2d:%.2d", hour, min, sec);

		//Writing Kernel Info to the Buffer: PID TTY TIME CMD
		if(task->signal->tty == NULL)
		{
			snprintf(kernel_buffer+length,
			  sizeof(kernel_buffer)-length,
			  " %d %s    %s %s \n",
			  task->pid,"?", timeBuffer, task->comm);
		}
		else
		{
			snprintf(kernel_buffer+length,
			  sizeof(kernel_buffer)-length,
			  " %d %s    %s %s \n",
			  task->pid, task->signal->tty->name, timeBuffer, task->comm);
		}

		//Updating the Length of the Buffer:
		length = strlen(kernel_buffer);
	}

	//If the Buffer is larger than the Destination Buffer:
	if(length > b)
		length = b;

	//Check whether the copy_to_user is successful:
	n = copy_to_user(c, kernel_buffer, length);
	return n;
}
