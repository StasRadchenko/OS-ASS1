#include "types.h"
#include "user.h"

//;;;;;;;;;;;;;;;;;Define section;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
#define NUM_OF_PROCESS 15
#define MEDIUM_LOOP_CALC 1000000
#define LARGE_LOOP_CALC 20000000
#define MEDIUM_LOOP_IO 1000
#define LARGE_LOOP_IO 10000
//;;;;;;;;;;;;;;;;;END OF DEFINE SECTION;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
long int factorial(int dig)
{
    long int facto;
    if (dig>1)
        facto = dig * factorial(dig-1);
    else 
        facto = 1;
    
    return facto;
}


void medium_cpu_usage(){
	int j;
	int k = 0;
	for (j = 0; j<MEDIUM_LOOP_CALC; j++){
		k = k + factorial(30);
	}

}

void large_cpu_usage(){
	int j;
	int k = 0;
	for (j = 0; j<LARGE_LOOP_CALC; j++){
		k = k + factorial(30);
	}

}


void medium_io_usage(){
	int j;
	for(j = 0; j<MEDIUM_LOOP_IO; j++){
		printf(1,"%s\n","MEDIUM IO LOOP");
	}

}

void large_io_usage(){
	int j;
	for(j = 0; j<LARGE_LOOP_IO; j++){
		printf(1,"%s\n","LARGE IO LOOP");
	}

}

int main(int argc, char *argv[])
{


	int pids[NUM_OF_PROCESS];


	for(int i=0; i< NUM_OF_PROCESS; i++){
		int pid;
	    pid = fork();
		if(pid == 0){
			#ifdef CFSD
				set_priority(i%3 + 1);
			#endif
			if(i%4 == 0)
				medium_cpu_usage(); //Calculation only - These processes will perform asimple calculation within a medium sized loop
			if(i%4 == 1)
				large_cpu_usage(); //Calculation only – These processes will perform simple calculation within a very large loop
			if(i%4 == 2)
				medium_io_usage();// Calculation + IO – These processes will perform printing to screen within a medium sized loop
			if(i%4 == 3)
				large_io_usage(); // Calculation + IO – These processes will perform printing to screen within a very large loop
				
			exit();	
		}
		else 
			pids[i] = pid;
	
	}


	int sum_wtime[4];
	int sum_rtime[4];
	int sum_iotime[4];
	int wtime;
	int rtime;
	int iotime;

	for(int i=0; i< NUM_OF_PROCESS; i++){
			wait2(pids[i],&wtime,&rtime,&iotime);
			sum_wtime[i%4] += wtime;
			sum_rtime[i%4] += rtime;
			sum_iotime[i%4] += iotime;
	   
	}
	

	printf(1,"Calculation Medium -  Wait time: %d,  Run time: %d, IO Time: %d\n\n",sum_wtime[0]/NUM_OF_PROCESS,sum_rtime[0]/NUM_OF_PROCESS,sum_iotime[0]/NUM_OF_PROCESS);
	printf(1,"Calculation Large -  Wait time: %d,  Run time: %d, IO Time: %d\n\n",sum_wtime[1]/NUM_OF_PROCESS,sum_rtime[1]/NUM_OF_PROCESS,sum_iotime[1]/NUM_OF_PROCESS);
	printf(1,"Calculation + IO Medium -  Wait time: %d,  Run time: %d, IO Time: %d\n\n",sum_wtime[2]/NUM_OF_PROCESS,sum_rtime[2]/NUM_OF_PROCESS,sum_iotime[2]/NUM_OF_PROCESS);
	printf(1,"Calculation + IO Large -  Wait time: %d,  Run time: %d, IO Time: %d\n\n",sum_wtime[3]/NUM_OF_PROCESS,sum_rtime[3]/NUM_OF_PROCESS,sum_iotime[3]/NUM_OF_PROCESS);

	exit();
}



