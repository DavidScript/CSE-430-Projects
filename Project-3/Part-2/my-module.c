/***************************************************************************\
* Project 3: Step 2 - Thrashing Alert					    *
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
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/rmap.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/rwsem.h>
#include <linux/unistd.h>

//Data:
typedef struct{
	struct task_struct *task;
	int counter;
	int valid ;	//0 - invalid | 1 - valid
	int pid;
} TaskWorkingSet;

//Function Prototype:
int my_kthread_function(void *data);
void run_app(void);
void init_vars(void);
void process_task_pte(struct task_struct *task, int pid);
int calc_pte(struct task_struct *task, unsigned long address);

//Global Variables:
int taskSize = 10000;				//How many Taks to keep track of
TaskWorkingSet taskCount[10000];		//Keep Track of Pointers and Working Set Counter
long long thrashingThreshold = 659180;		//Number to Alert for Thrashing
struct task_struct* myThread;			//Keeping Track of My Kernel Thread

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

//Kernel Thread:
int my_kthread_function(void *data){

	//Looping Forever:
	while(!kthread_should_stop()){

		//Sleep for 1000 milliseconds: (1 second)
		msleep(1000);

		//Running the Application:
		run_app();
	}

	return 0;
}

//Running the Application:
void run_app(void){

	//Variable Declaration:
	struct task_struct *task;
	unsigned long long twss;
	int i;

	//Initializing Variables:
	twss = 0;

	//Initializing Variables:
	init_vars();

	//For Each Task Find the [PID]:[WSS]
	for_each_process(task){

		//Gathering Processes:
		taskCount[task->pid].task = task;
		taskCount[task->pid].pid = task->pid;

		//Weaving Out Valid PIDs that Are Mapped:
		if(task != NULL && task->mm != NULL && task->mm->mmap != NULL){
			taskCount[task->pid].valid = 1;
		}
	}

	//Calculating Working Set:
	for(i = 0; i != taskSize; i++){
		if(taskCount[i].valid == 1 && taskCount[i].pid > 0){
			process_task_pte(taskCount[i].task, taskCount[i].pid);
		}
	}

	//Calculating Total Working Set: [Total WWS]:[TWSS]
	for(i = 0; i != taskSize; i++){
		if(taskCount[i].counter > 0 && taskCount[i].valid == 1 && taskCount[i].pid > 0){
			//printk("[PID: %d]:[WSS: %d]\n", taskCount[i].pid, (taskCount[i].counter * PAGE_SIZE));
			twss += taskCount[i].counter;
		}
	}

	//Printing out the TWSS:
	printk("[TWSS]: %llu\n", (twss * PAGE_SIZE));

	//Detecting Thrashing:
	if(twss >= thrashingThreshold){
		printk(KERN_EMERG "Kernel Alert!\n");
	}
}

//Initializing Variables:
void init_vars(void)
{
	//Variable Declaration:
	int i = 0;

	while(i != taskSize){
		taskCount[i].counter = 0;
		taskCount[i].pid = 0;
		taskCount[i].valid = 0;
		i++;
	}
}

//Given a Task: Look up in the Dictionary:
void process_task_pte(struct task_struct *task, int pid){

	//Variable Declaration:
	struct vm_area_struct *vma;
	unsigned long vfn;

	if(task != NULL && task->mm != NULL && task->mm->mmap != NULL){

		//Working Set: (VMA: Virtual Memory Address)
		for(vma = task->mm->mmap; vma != NULL; vma = vma->vm_next){

			//Page Table walk with pgd_offset(my_task->mm, vfn)
			for(vfn = vma->vm_start; vfn < vma->vm_end; vfn = vfn + PAGE_SIZE)
			{
				//Counter: (my_task, address)
				taskCount[pid].counter += calc_pte(task, vfn);
			}
		}
	}
}

//Given a Task->mm, Virtual Address: Table Walk and Find the PTE (0 or 1)
int calc_pte(struct task_struct *task, unsigned long address){

	//Variable for valid:
	int result;

	//Variables used for Page Walking:
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep, pte;

	//Initializing Variables:
	pgd = NULL;
	pud = NULL;
	pmd = NULL;
	ptep = NULL;
	pte = *ptep;
	result = 0;

	//Finding the First Offset PGD:
	if(task == NULL || task->mm == NULL){
		return 0;
	}else{
		pgd = pgd_offset(task->mm, address);
	}

	//Finding the Second Offset PUD:
	if(pgd == NULL || pgd_none(*pgd) || pgd_bad(*pgd)){
		return 0;
	}else{
		pud = pud_offset(pgd, address);
	}

	//Finding the Third Offset PMD:
	if(pud == NULL || pud_none(*pud) || pud_bad(*pud)){
		return 0;
	}else{
		pmd = pmd_offset(pud, address);
	}

	//Finding the PTE:
	if(pmd == NULL || pmd_none(*pmd) || pmd_bad(*pmd)){
		return 0;
	}else{
		ptep = pte_offset_map(pmd, address);
		pte = *ptep;

		//Checking that the Page is used:
		if(pte_present(pte)){
			//Being Used:
			if(pte_young(pte)){
				result = 1;
			}

			//Make Dirty:
			pte_mkold(pte);
		}

		//Always Unmap:
		pte_unmap(ptep);
	}

	return result;
}
