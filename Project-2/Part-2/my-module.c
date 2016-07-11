/***************************************************************************\
* Project 2: Step 2 - Notifying and Killing Fork Bombs    		    *
*    David McKay							    *
\***************************************************************************/
//Note: How to Run and Bash Commands:
// Kernel Modules:
//   Load:	sudo insmod ./my-module.ko
//   Unload:	sudo rmmod my-module
// Kernel Log:
//   View:	nano /var/log/kernel.log
//   Clear:	sudo tee /var/log/kernel.log </dev/null

//Headers:
#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/delay.h>

//Data:
typedef struct{
	struct task_struct *task;
	int counter;
} TaskCounter;

//Function Prototype:
int my_kthread_function(void *data);
void my_name(void);
void init_vars(void);
void scan_for_fork_bomb(void);
void defuse_fork_bomb(void);
void kill_process_family(struct task_struct *t);

//Global Variables: (Could Change the Numbers if Required)
int taskSize = 50000;		//How many Tasks to keep track of
TaskCounter taskCount[50000];	//Keep Track of Pointers of the Task and Child Counter
int importantPID = 1500;	//Don't kill Important PIDs
int forkBombThreshold  = 100; 	//Don't Allow more than 100 Child Processes
struct task_struct* myThread;	//Keeping Track of My Kernel Thread

//Init Module:
int init_module(void)
{
	//Variable Declaration:
	int data = 20;

	//Start this Module as a Thread:
	myThread = kthread_run( &my_kthread_function,
				(void *) &data,
				"my_kthread");

	return(0);
}

//Cleanup Module:
void cleanup_module(void)
{
	kthread_stop(myThread);
}

//Implementing the Function Prototype: (Down below)

//Implementing my Thread:
int my_kthread_function(void *data)
{
	while(!kthread_should_stop()){

		//Stopping Blocks:
		preempt_disable();

		//Sleep for 30000 milliseconds: (30 seconds)
		msleep(30000);

		//Initializing Variables:
		init_vars();

		//Scanning all The Processes: (Counting Child Size)
		scan_for_fork_bomb();

		//Detect the Fork Bomb: (KIlling all the Processes)
		defuse_fork_bomb();

		//Allowing Interupts:
		preempt_enable();
	}

	return 0;
}

//Initializing Variables:
void init_vars(void)
{
	//Variable Declaration:
	int i = 0;

	while(i != taskSize){
		taskCount[i].counter = 0;

		//DEBUG: Ensuring all Counters set to 0
		//printk("%d\n", taskCount[i].counter);

		i++;
	}
}

//Count up all the Childs for Each process:
void scan_for_fork_bomb(void)
{
	//Variable Declaration:
	struct task_struct *task;
	struct task_struct *myTask;

	//Traverse Each Process:
	for_each_process(task){

		//Gathering pointers: (Killing them, if they are bombs)
		taskCount[task->pid].task = task;

		//Increment the Processes that each Ancestor Has:
		for(myTask = task; myTask != &init_task; myTask = myTask->parent){

			//Incrementing Counter:
			taskCount[myTask->pid].counter++;
		}
	}
}

//Detecting if a Fork Bomb Exist: (Killing It)
void defuse_fork_bomb(void)
{
	//Variable Declaration:
	int i = 0;

	//Scanning for Kernel Bomb: (Killing the Bomb)
	while(i != taskSize){
		if(taskCount[i].counter > 0 && taskCount[i].task->pid > importantPID && 
		   strcmp(taskCount[i].task->comm, "bash")!=0 && taskCount[i].counter >= forkBombThreshold){

			//DEBUG: Printing out the Fork Bomb Main Process:
			//printk("[%d] pid:%d cmd:%s\n", taskCount[i].counter, i, taskCount[i].task->comm);
			kill_process_family(taskCount[i].task);

		}

		i++;
	}
}

//Killing the Family Tree:
void kill_process_family(struct task_struct *t)
{
	//Variable Declaration:
	struct list_head* pos;

	//Iterate through all the Child Process:
	list_for_each(pos, &t->children){
		struct task_struct *c = list_entry(pos, struct task_struct, sibling);
		kill_process_family(c);
	}

	//Kill each Child Process:
	printk("%d\n", t->pid);

	//DEBUG: More Info on the Process
	//printk("[%d] pid:%d cmd:%s\n", taskCount[t->pid].counter, t->pid, t->comm);
	send_sig(SIGKILL, t, 1);
}
