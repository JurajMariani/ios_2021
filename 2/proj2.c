//---------------------------------------------------------
//		@AUTHOR: 	JURAJ MARIANI
//		<email>:	xmaria03@stud.fit.vutbr.cz
//
//		Structure:	Includes / Macros / Global Variables
//					------------------------------------
//									MAIN
//					------------------------------------
//								 FUNCTIONS
//
//---------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>
#include "santa_and_helpers.h"

#define LOCK 0
#define UNLOCK 1
#define WAIT_TIME_NUMBER 3
// Error macro
#define err(s) fprintf(stderr, "%s\n",s);
// Message macro for processes ELF and REINDEER
#define msg(name,id,message) if (sem_wait(semaphore_shared_lock) == -1) {*error_flag = true; err("Error 'Semaphore shared lock'");} fprintf(stdout,"%d: %s %d: %s\n",++(*shared_counter),name,id,message); fflush(stdout); if(sem_post(semaphore_shared_lock) == -1) {*error_flag = true; err("Error 'Semaphore shared lock'");}
// Message macro for process SANTA
#define msg_s(message) if (sem_wait(semaphore_shared_lock) == -1) {*error_flag = true; err("Error 'Semaphore shared lock'");} fprintf(stdout,"%d: Santa: %s\n",++(*shared_counter),message); fflush(stdout); if (sem_post(semaphore_shared_lock) == -1) {*error_flag = true; err("Error 'Semaphore shared lock'");}

//-----------------------------------------------------
// GLOBAL (please don't kill me) VARIABLE AREA

#define SEM_ASSIGN "/ios_p2_xmaria03_process_assign"
sem_t* semaphore_assign = NULL;

int shared_counter_id = 0;
int* shared_counter = NULL;

int process_number_id = 0;
int* process_number = NULL;

#define SEM_HITCH "/ios_p2_xmaria03_process_hitching"
sem_t* semaphore_hitching = NULL;

int hitching_request_id = 0;
int* hitching_request = NULL;

#define SEM_HITCH_ACTIVE "/ios_p2_xmaria03_process_active_hitching"
sem_t* semaphore_active_hitching = NULL;

#define SEM_QUEUE "/ios_p2_xmaria03_process_queue"
sem_t* semaphore_queue = NULL;

#define SEM_GET_HELP "/ios_p2_xmaria03_process_get_help"
sem_t* semaphore_get_help = NULL;

#define SEM_WAIT "/ios_p2_xmaria03_process_wait_time_ahead"
sem_t* semaphore_wait_time_ahead = NULL;

#define SEM_SANTA_SLEEP "/ios_p2_xmaria03_process_santa_sleep"
sem_t* semaphore_santa_sleep = NULL;

#define SEM_SHARED_LOCK "/ios_p2_xmaria03_process_shared_lock_counter"
sem_t* semaphore_shared_lock = NULL;

int christmas_vacation_id = 0;
bool* christmas_vacation = NULL;

int error_flag_id = 0;
bool* error_flag = NULL;

int write_to_file;

process_t process;

enum arg_order{Elves, Reindeers, Alone_Time, Vac_Time};

// Variable array containing all the child pids
int* playground = NULL;

//---------------------------------------------------

int element_check(int* init);
int arg_num_check(char* arg);
bool set_shared(void);
void unset_shared(void);
void unset_assign(void);
void create_processes(int* safe);
void santa(int* safe);
void elf(int* safe);
void reindeer(int* safe);
void get_hitched(void);

//---------------------------------------------------

int main (int argc, char** argv)
{
	if ( argc != 5 )
	{
		err("There have to be exactly 4 parameters");
		return 1;
	}

	int safekeeping[4] = {0,0,0,0};	/** the array with the command line arguments */
	for (int i = 1 ; i < 5 ; i++)
	{
		if ( arg_num_check( argv[i] ) )
		{
			err("Argument NAN");
			return 1;
		}
		safekeeping[i - 1] = atoi(argv[i]);
	}

	if (element_check(safekeeping))
	{
		return 1;
	}

	// +1 at the end serves as a bumper
	int count = 1 + safekeeping[Elves] + safekeeping[Reindeers] + 1;
	playground = malloc(count * sizeof(int));
	if (playground == NULL)
	{
		err("Memory Allocation failure");
		return 1;
	}
	for (int f = 0 ; f < count ; f++)
		playground[f] = 0;


	// The standard output buffer is set to null, but I still use fflush
	setbuf(stdout, NULL);

	if ( set_shared() )
	{
		free(playground);
		unset_shared();
		unset_assign();
		return 1;
	}

	// Rerouting STDOUT to file 'proj2.out'
	write_to_file = open("proj2.out", O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0777 );
	if (write_to_file == -1)
	{
		err("File 'proj2.out' cannot be opened");
		free(playground);
		unset_shared();
		unset_assign();
		return 1;
	}
	dup2(write_to_file, STDOUT_FILENO);
	close(write_to_file);

	// Setting the default process name and id to zero
	process.name = "0";
	process.ID = 0;

	// Creating Santa(1), Elves(NE) and Reindeers(NR)
	create_processes(safekeeping);

	if ( strcmp( process.name, "Elf") == 0 )
		elf(safekeeping);
	else
		if ( strcmp( process.name, "RD") == 0 )
			reindeer(safekeeping);
		else
			if ( strcmp( process.name, "Santa") == 0 )
				santa(safekeeping);

	// Wait for child processes to finish, if the error flag is true, kill all children :)	** Don't worry, no going to jail**
	while((wait(NULL) != -1) || ( errno != ECHILD))
		if ((process.ID == 0) && (*error_flag))
			for (int g = 0 ; playground[g] != 0 ; g++)
				kill(playground[g], SIGTERM);


	if (process.ID == 0)
	{
		// Semaphore, Shared memory and Playground destructor
		free(playground);
		unset_assign();
		unset_shared();
	}

	if (*error_flag)
		return 1;
	return 0;

}


// Check if all arguments are numbers
// Returns: 0(OK), 1(ERROR)
int arg_num_check(char* arg)
{
	for (int i = 0 ; arg[i] != '\0' ; i++)
	{
		if (( arg[i] < '0' ) || ( arg[i] > '9' ))
			return 1;
	}
	return 0;
}


// Checks if the given argument is within the interval
// Returns: 0(OK) [position of error](ERROR)
int element_check(int* init)
{

	if (( init[Elves] < 1 ) || ( init[Elves] >=1000 ))
	{
		err("Wrong Elf count, correct interval (0,1000) / <1,999>");
		return Elves + 1;
	}

	if (( init[Reindeers] < 1) || ( init[Reindeers] >= 20))
	{
		err("Wrong Reindeer count, correct interval (0,20) / <1,19>");
		return Reindeers + 1;
	}

	if (( init[Alone_Time] < 0 ) || ( init[Alone_Time] > 1000 ))
	{
		err("Wrong Elf Alone Time, correct interval <0,1000>");
		return Alone_Time + 1;
	}

	if (( init[Vac_Time] < 0) || ( init[Vac_Time] > 1000))
	{
		err("Wrong Reindeer count, correct interval <0,1000>");
		return Vac_Time + 1;
	}

	return 0;
}


// Opens all the Semaphores and Shared Memories
// FALSE(OK), TRUE(ERROR)
bool set_shared(void)
{

	if ( (semaphore_assign = sem_open(SEM_ASSIGN, O_CREAT | O_EXCL, 0666 , UNLOCK)) == SEM_FAILED )
	{
		err("Semaphore 'assign' failed to open");
		return true;
	}

	if ( (semaphore_hitching = sem_open(SEM_HITCH, O_CREAT | O_EXCL, 0666 , UNLOCK)) == SEM_FAILED )
	{
		err("Semaphore 'hitching' failed to open");
		return true;
	}

	if ( (semaphore_active_hitching = sem_open(SEM_HITCH_ACTIVE, O_CREAT | O_EXCL, 0666 , LOCK)) == SEM_FAILED )
	{
		err("Semaphore 'active_hitching' failed to open");
		return true;
	}

	if ( (semaphore_queue = sem_open(SEM_QUEUE, O_CREAT | O_EXCL, 0666 , WAIT_TIME_NUMBER)) == SEM_FAILED )
	{
		err("Semaphore 'queue' failed to open");
		return true;
	}

	if ( (semaphore_get_help = sem_open(SEM_GET_HELP, O_CREAT | O_EXCL, 0666 , LOCK)) == SEM_FAILED )
	{
		err("Semaphore 'get_help' failed to open");
		return true;
	}

	if ( (semaphore_wait_time_ahead = sem_open(SEM_WAIT, O_CREAT | O_EXCL, 0666 , UNLOCK)) == SEM_FAILED )
	{
		err("Semaphore 'wait_time_ahead' failed to open");
		return true;
	}

	if ( (semaphore_santa_sleep = sem_open(SEM_SANTA_SLEEP, O_CREAT | O_EXCL, 0666 , LOCK)) == SEM_FAILED )
	{
		err("Semaphore 'santa_sleep' failed to open");
		return true;
	}

	if ( (semaphore_shared_lock = sem_open(SEM_SHARED_LOCK, O_CREAT | O_EXCL, 0666 , UNLOCK)) == SEM_FAILED )
	{
		err("Semaphore 'shared_lock' failed to open");
		return true;
	}


	if ((shared_counter_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
	{
		err("Shared memory ID 'shared_counter_id' failed");
		return true;
	}

	if ((shared_counter = shmat(shared_counter_id, NULL, 0)) == NULL)
	{
		err("Shared memory 'shared_counter' failure");
		return true;
	}
	*shared_counter = 0;


	if ((hitching_request_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
	{
		err("Shared memory ID 'hitching_request_id' failed");
		return true;
	}

	if ((hitching_request = shmat(hitching_request_id, NULL, 0)) == NULL)
	{
		err("Shared memory 'hitching_request' failure");
		return true;
	}
	*hitching_request = 0;


	if ((christmas_vacation_id = shmget(IPC_PRIVATE, sizeof(bool), IPC_CREAT | 0666)) == -1)
	{
		err("Shared memory ID 'christmas_vacation_id' failed");
		return true;
	}

	if ((christmas_vacation = shmat(christmas_vacation_id, NULL, 0)) == NULL)
	{
		err("Shared memory 'christmas_vacation' failure");
		return true;
	}
	*christmas_vacation = false;


	if ((error_flag_id = shmget(IPC_PRIVATE, sizeof(bool), IPC_CREAT | 0666)) == -1)
	{
		err("Shared memory ID 'error_flag_id' failed");
		return true;
	}

	if ((error_flag = shmat(error_flag_id, NULL, 0)) == NULL)
	{
		err("Shared memory 'error_flag' failure");
		return true;
	}
	*error_flag = false;

	if ((process_number_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
	{
		err("Shared memory ID 'process_number_id' failed");
		return true;
	}

	if ((process_number = shmat(process_number_id, NULL, 0)) == NULL)
	{
		err("Shared memory 'process_counter' failure");
		return true;
	}
	*process_number = 0;


	return false;
}


// Destructor for Semaphore Assign
void unset_assign()
{

	sem_close(semaphore_assign);
	sem_unlink(SEM_ASSIGN);

}


// Semaphore and Shared memory destructor
void unset_shared(void)
{
	
	sem_close(semaphore_hitching);
	sem_unlink(SEM_HITCH);

	sem_close(semaphore_active_hitching);
	sem_unlink(SEM_HITCH_ACTIVE);

	sem_close(semaphore_queue);
	sem_unlink(SEM_QUEUE);

	sem_close(semaphore_get_help);
	sem_unlink(SEM_GET_HELP);

	sem_close(semaphore_wait_time_ahead);
	sem_unlink(SEM_WAIT);

	sem_close(semaphore_santa_sleep);
	sem_unlink(SEM_SANTA_SLEEP);

	sem_close(semaphore_shared_lock);
	sem_unlink(SEM_SHARED_LOCK);

	shmctl(shared_counter_id, IPC_RMID, NULL);

	shmctl(process_number_id, IPC_RMID, NULL);

	shmctl(hitching_request_id, IPC_RMID, NULL);

	shmctl(christmas_vacation_id, IPC_RMID, NULL);

	shmctl(error_flag_id, IPC_RMID, NULL);

	return;
}


// Creates all processes
void create_processes(int* safe)
{
	
	int id = 1;

	int sum = 1 + safe[Elves] + safe[Reindeers];
	unsigned int i = 0;

	// Main process is here forked [Santa + Elves + Reindeers] times
	while((id > 0) && ( (int)i < sum ))
	{
		id = fork();
		if (id != 0)
			playground[i] = id;

		if ((*error_flag) && (id != 0))
			return;

		i++;
	}

	if (id == -1)
	{
		err("Could not fork");
		*error_flag = true;
		exit(1);
	}

	// Only child processes may enter
	if (id == 0)
	{
		// Entering critical section - process number incrementation
		if (sem_wait(semaphore_assign) == -1)
			{*error_flag = true; err("Semaphore Failure");}
		
		int num_of_process = ++(*process_number);
		
		if (sem_post(semaphore_assign) == -1)
			{*error_flag = true; err("Semaphore Failure");}
		// End of critical section

		if (num_of_process == 1)	/** Santa */
			init_santa(&process);

		if ((num_of_process > 1) && (num_of_process <= 1 + safe[Elves] ))	/** Elf */
			init_elf(&process, (num_of_process - 1) );

		if ((num_of_process > 1 + safe[Elves] ) && (num_of_process <= sum ))	/** Reindeer */
			init_reindeer(&process, (num_of_process - 1 - safe[Elves]) );
		
		return;
	}

	// Here goes only the parent process
	// Unsets the Assign Semaphore and process_number shared memory
	if (usleep(1000) == -1) /** Unnecessary? 1ms wait */
		{*error_flag = true; err("Usleep Failure");}

	unset_assign();
	shmctl(process_number_id, IPC_RMID, NULL);

	return;
}


// ***** Santa Process *****
void santa(int* safe)
{
	// Init message - "Going to sleep"
	msg_s(process.init_msg);

	int value = 1, i;
	while(1)
	{	
		// If the value of Semaphore Queue is zero - there are three Elves waiting ...
		if (sem_getvalue(semaphore_queue, &value) == -1)
			{*error_flag = true; err("Semaphore Failure");}
		if ( value == 0 )
		{
			// Print help message - "Helping Elves"
			msg_s(process.help_msg);

			// Post Elves's Semaphore three times
			for (i = 0 ; i < 3 ; i++)
				if (sem_post(semaphore_get_help) == -1)
					{*error_flag = true; err("Semaphore Failure");}
			// Wit for the Elves to print "got help" before printing init message again
			if (sem_wait(semaphore_santa_sleep) == -1)
				{*error_flag = true; err("Semaphore Failure");}
			msg_s(process.init_msg);
		}

		if (sem_getvalue(semaphore_queue, &value) == -1)
			{*error_flag = true; err("Semaphore Failure");}
		// If all the Reindeers are home
		if ((*hitching_request == safe[Reindeers]) && (value != 0))
		{
			// Print the done message - "Closing Workshop"
			msg_s(process.done_msg);
			// Set the Christmas vacation Shared Memory to TRUE
			*christmas_vacation = true;
			
			// Mrs. Clause is forked
			int santa_id = fork();

			// Santa is hitching the Reindeers
			if(santa_id != 0)
			{
				for(int j = 0 ; j < safe[Reindeers] ; j++)
				{
					*hitching_request -= 1;
					if (sem_post(semaphore_active_hitching) == -1)
						{*error_flag = true; err("Semaphore Failure");}
				}
				
				if (sem_wait(semaphore_santa_sleep) == -1)
					{*error_flag = true; err("Semaphore Failure");}

				msg_s(process.end_msg);
				break;
			}
			else
			{
				// ***Mrs. Clause ONLY***
				// Mrs. Clause is posting the Elves's Semaphore, so they can go on vacation
				for (int x = 0 ; x < 3 ; x++)
					if (sem_post(semaphore_get_help) == -1)
						{*error_flag = true; err("Semaphore Failure");}

				exit(0);
			}
		}

	}

	return;
}


// ***** Elf Process *****
void elf(int* safe)
{
	// Init message - "Started"
	msg(process.name, process.ID, process.init_msg);

	int value;
	while(1)
	{
		// Generate random WorkTime
		srand(time(NULL) * getpid());
		long alone_time = (rand() % (safe[Alone_Time] + 1) * 1000); // multiply by 1000, because usleep accepts time in microseconds

		// Elves slee* ehm, I mean WORK here 
		if (usleep(alone_time) == -1)
			{*error_flag = true; err("Semaphore Failure");}

		// Help message - "Need Help"
		msg(process.name,process.ID, process.help_msg);
	
		if (sem_wait(semaphore_wait_time_ahead) == -1)
			{*error_flag = true; err("Semaphore Failure");}
		if (sem_wait(semaphore_queue) == -1)
			{*error_flag = true; err("Semaphore Failure");}

		if (sem_getvalue(semaphore_queue, &value) == -1)
			{*error_flag = true; err("Semaphore Failure");}

		if (value > 0)
			if (sem_post(semaphore_wait_time_ahead) == -1)  /** If the value of the first semaphore "wait time ahead", let enother Elf in. */
				{*error_flag = true; err("Semaphore Failure");}

		// YAY, Vacation! But, before the Elves go, they help other Elves to go to Vacation
		if (*christmas_vacation == true)
		{
			// Print end message - "Taking Holidays"
			msg(process.name, process.ID, process.end_msg);
			if (sem_post(semaphore_queue) == -1)
				{*error_flag = true; err("Semaphore Failure");}
			if (sem_post(semaphore_wait_time_ahead) == -1)
				{*error_flag = true; err("Semaphore Failure");}
			return;
		}
		
		// Waiting for Santa's help
		if (sem_wait(semaphore_get_help) == -1)
			{*error_flag = true; err("Semaphore Failure");}

		// YAY, Vacation! But, before the Elves go, they help other Elves to go to Vacation
		if (*christmas_vacation == true)
		{
			// Print end message - "Taking Holidays"
			msg(process.name, process.ID, process.end_msg);
			if (sem_post(semaphore_queue) == -1)
				{*error_flag = true; err("Semaphore Failure");}
			if (sem_post(semaphore_wait_time_ahead) == -1)
				{*error_flag = true; err("Semaphore Failure");}
			return;
		}

		// Print done message -  "Got Help"
		msg(process.name, process.ID, process.done_msg);

		if (sem_post(semaphore_queue) == -1)
			{*error_flag = true; err("Semaphore Failure");}

		// The last process posts the Wait Time Ahead
		if (sem_getvalue(semaphore_queue, &value) == -1)
			{*error_flag = true; err("Semaphore Failure");}
		if (value == 3)
		{
			if (sem_post(semaphore_wait_time_ahead) == -1)
				{*error_flag = true; err("Semaphore Failure");}
			// Let santa write the init message - "Going To Sleep"
			if (sem_post(semaphore_santa_sleep) == -1)
				{*error_flag = true; err("Semaphore Failure");}
		}

	}

	return;
}


// ***** Reindeer Process *****
void reindeer(int* safe)
{
	// Print the init message - "RStarted"
	msg(process.name,process.ID, process.init_msg);

	// Generate random Vavation Time
	srand(time(NULL) * getpid());
	long vac_time = ((rand() % ((safe[Vac_Time] + 1) / 2)) + (safe[Vac_Time] / 2) ) * 1000; // multiply by 1000, because usleep accepts time in microseconds

	// Reindeers enjoy Vacation here
	if (usleep(vac_time) == -1)
		{*error_flag = true; err("Usleep Failure");}

	// Print done message - "Return Home"
	msg(process.name, process.ID, process.done_msg);
	// Get Hitched
	get_hitched();
	// Print end message - "Get Hitched"
	msg(process.name, process.ID, process.end_msg);
	
	return;
}


// Get Hitched reindeer helper function
void get_hitched(void)
{

	if (sem_wait(semaphore_hitching) == -1)
		{*error_flag = true; err("Semaphore Failure");}
	// Critical section where the Reindeers increment the hitching request Shared Memory
	*hitching_request += 1;
	// End of Critical Section
	if (sem_post(semaphore_hitching) == -1)
		{*error_flag = true; err("Semaphore Failure");}

	// here will the reindeers end up, only when santa gives them permission by raising the value of the semaphore
	if (sem_wait(semaphore_active_hitching) == -1)
		{*error_flag = true; err("Semaphore Failure");}
	// The last hitched Reindeer posts Santa's semaphore
	if (*hitching_request == 0)
		if (sem_post(semaphore_santa_sleep) == -1)
			{*error_flag = true; err("Semaphore Failure");}
	return;
}

/*** END OF CODE proj2.c ***/
