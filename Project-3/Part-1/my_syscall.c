//CSE 430: Project 3 - Address Translation
//Date: 4/3/2016

//David McKay

//Headers:
#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/tty.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/rmap.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/rwsem.h>

//Data:
typedef struct{
    int pid;
	char tty[64];
	int hour;
	int min;
	int sec;
	char cmd[64];
	int result; //0 -invalid | 1 - physical | 2 - swap
	unsigned long pfn;
	unsigned long long swap;
} myProcessInfo;

//Project 3: Address Translation
//int a: initial starting point 0
//int b: sizeof(UserSpace_Buffer)
//char* c: Userspace_Buffer
//int myPID: PID
//int myAddress: Address
asmlinkage long sys_my_syscall(int a, int b, char *c, int myPID, unsigned long myAddress)
{
	//Initializing Variables:
	struct task_struct *task;
	struct task_struct *my_task;
	unsigned long jiff;
	cputime_t utime;
	cputime_t stime;
	int n = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;
	int userLength = 0;

	myProcessInfo myPI;

	//Variables Page Walking:
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep, pte;
	swp_entry_t entry;

	//Initializing Variables:
	pgd = NULL;
	pud = NULL;
	pmd = NULL;
	ptep = NULL;
	pte = *ptep;

	myPI.pid = 0;
	myPI.hour = 0;
	myPI.min = 0;
	myPI.sec = 0;
	myPI.result = 0;
	myPI.pfn = 0;
	myPI.swap = 0;

	my_task = NULL;

	//Step 1: Find the process with the PID
	for_each_process(task){
		if(task->pid == myPID){
			my_task = task;
			//Calculating the Time for the Task:
			thread_group_cputime_adjusted(task, &utime, &stime);
			jiff = utime + stime;
			jiff = jiff/1000;
			hour = jiff/3600;
			min = (jiff-(hour*3600))/60;
			sec = (jiff-(hour*3600)-(min*60));

			//Writing Kernel Info to the Buffer: PID TTY TIME CMD
			if(task->signal->tty == NULL)
			{
				//Converting everything into Struct:
				myPI.pid = task->pid;
				sprintf(myPI.tty, "%s", "?");
				myPI.hour = hour;
				myPI.min = min;
				myPI.sec = sec;
				sprintf(myPI.cmd, "%s", task->comm);
			}
			else
			{
				//Converting everything into Struct:
				myPI.pid = task->pid;
				sprintf(myPI.tty, "%s", task->signal->tty->name);
				myPI.hour = hour;
				myPI.min = min;
				myPI.sec = sec;
				sprintf(myPI.cmd, "%s", task->comm);
			}
		}
	}

	//Step 2: Finding the first offset for the PGD:
	if(my_task != NULL && my_task->mm != NULL){
		pgd = pgd_offset(my_task->mm, myAddress);
	}else{
		myPI.result = 0;
		//Saving the Results to the Userspace:
		if((b - userLength) >0){
			n = copy_to_user(c + userLength, (char*) &myPI, b- userLength);
			userLength += sizeof(myProcessInfo);
			a++;
		}
		return -1;
	}

	//Step 3: Finding the second offset for the PUD:
	if(pgd == NULL || pgd_none(*pgd) || pgd_bad(*pgd)){
		//printk("PGD - Page not Found\n");
		myPI.result = 0;
		//Saving the Results to the Userspace:
		if((b - userLength) >0){
			n = copy_to_user(c + userLength, (char*) &myPI, b- userLength);
			userLength += sizeof(myProcessInfo);
			a++;
		}
		return -1;
	}else{
		pud = pud_offset(pgd, myAddress);
	}

	//Step 4: Finding the third offset for the PMD:
	if(pud == NULL || pud_none(*pud) || pud_bad(*pud)){
		myPI.result = 0;
		//Saving the Results to the Userspace:
		if((b - userLength) >0){
			n = copy_to_user(c + userLength, (char*) &myPI, b- userLength);
			userLength += sizeof(myProcessInfo);
			a++;
		}
		return -1;

	}else{
		pmd = pmd_offset(pud, myAddress);
	}

	//Step 5: Finding Virtual or Physicall Address for the PTE:
	if(pmd == NULL || pmd_none(*pmd) || pmd_bad(*pmd)){
		myPI.result = 0;
		//Saving the Results to the Userspace:
		if((b - userLength) >0){
			n = copy_to_user(c + userLength, (char*) &myPI, b- userLength);
			userLength += sizeof(myProcessInfo);
			a++;
		}
		return -1;

	}else{
		ptep = pte_offset_map(pmd, myAddress);
		pte = *ptep;
		if(pte_present(pte)){
			//Physicall Address:
			myPI.result = 1;
			myPI.pfn = pte_pfn(pte);
		}else{
			//Logical Address:
			myPI.result = 2;

			//Find the swap identifier:
			entry = pte_to_swp_entry(pte);
			myPI.swap = entry.val;
		}
		pte_unmap(ptep);
	}

	//Last Step:
	//Saving the Results to the Userspace:
	if((b - userLength) >0){
		n = copy_to_user(c + userLength, (char*) &myPI, b- userLength);
		userLength += sizeof(myProcessInfo);
		a++;
	}

	return a;
}
