#include <stdio.h>
#include <stdlib.h>		// For exit() and srand()
#include <unistd.h>		// For fork() and close()
#include <sys/wait.h>	// For wait()
#include <errno.h>		// Guess
#include <pthread.h>	// Testing with threads
#include <semaphore.h>	// For use with threads
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>		// For time(), used in srand()
#include "snake.h"		// For the ioctl functions
#include <sys/ioctl.h>

// Set this to 1 if you want to see the output of PRINT
#define HW4_TEST_DEBUG 0

// --------------------------------IMPORTANT------------------------------------------------
// These macros / functions need to be redefined for your code:
#define INSTALL_SCRIPT "/mnt/hgfs/SharedFolder/os_wet4/install.sh"
#define UNINSTALL_SCRIPT "/mnt/hgfs/SharedFolder/os_wet4/uninstall.sh"
char node_name[20];
char* get_node_name(int minor) {
	sprintf(node_name,"/dev/snake%d",minor);
	return node_name;
}
// I'm assuming the scripts are called like this:
//
//	./install.sh 6		// Does insmod and mknod * 6 (creates snake0,snake1,...,snake6 in /dev/)
//	./uninstall.sh		// Does rmmod and deletes created files	(rm -f /dev/snake*)
//
// Where '6' is the max number of games (see setup_snake()).
// To use this, write a script that behaves like the one above. Here is ours:
/* *********************************** install.sh ***************************************** 
#!/bin/bash

# Go to the correct directory
cd /root/hw4

# Make sure the input is OK - we expect to get N (total number of games allowed)
if [ "$#" -ne 1 ]; then
	echo "Need to provide an argument. Usage: install.sh [TOTAL_GAMES]"; exit 1
fi
if ! [[ $1 != *[!0-9]* ]]; then
   echo "Error: argument not a number. Usage: install.sh [TOTAL_GAMES]"; exit 2
fi

# Install the module (no cleanup or building)
insmod ./snake.o max_games=$1

# Acquire the MAJOR number
major=`cat /proc/devices | grep snake | sed 's/ snake//'`

# Create the files for the games
i=0
while [ $i -lt $1 ]; do
	mknod /dev/snake$i c $major $i
	let i=i+1
done
*************************************** install.sh ****************************************/
/* *********************************** uninstall.sh ****************************************
#!/bin/bash

# Go to the correct directory
cd /root/hw4

# Do some cleanup
rm -f /dev/snake*
rmmod snake
************************************** uninstall.sh ***************************************/

/*******************************************************************************************
 ===========================================================================================
 ===========================================================================================
								GLOBALS & TYPEDEFS
 ===========================================================================================
 ===========================================================================================
 ******************************************************************************************/

// Error string output
char output[256];

// Boolean type
typedef char bool;
#define TRUE	((char)1)
#define FALSE	((char)0)

// Color types
#define WHITE_COLOR 4
#define BLACK_COLOR 2

// Data passed to threads in test functions.
typedef struct {
	sem_t* sem_arr;	// Used for thread "communication"
} ThreadParam;

// Debugging the tester (I know. Shut up.)
#if HW4_TEST_DEBUG
	#define PRINT(...) printf(__VA_ARGS__)
	#define PRINT_IF(cond,...) if(cond) PRINT(__VA_ARGS__)
#else
	#define PRINT(...)
	#define PRINT_IF(...)
#endif

// This is set to TRUE if a process has forked using FORK() at the beginning of a test.
// That way, ASSERT()s know to call P_CLEANUP() before returning.
// Same goes for CLONE() and T_CLEANUP()
bool forked = FALSE;
bool cloned = FALSE;

// This is set to TRUE if the module has been installed with setup_snake().
// If you use ASSERT()s, this has to be set correctly!
// Just use setup_snake() and destroy_snake() for that,
// or better yet: SETUP_P() and DESTROY_P().
bool installed = FALSE;

// Use these for easier thread / process use. Used in macros.
pthread_t* tids=NULL;
int* pids=NULL;
int child_num=-1, total_threads=-1, total_semaphores=-1;
ThreadParam tp;

// Use these for test progress
int total_progress=-1;
int total_progress_indicators=-1;

/*******************************************************************************************
 ===========================================================================================
 ===========================================================================================
                            MACROS AND UTILITY FUNCTIONS
 ===========================================================================================
 ===========================================================================================
 ******************************************************************************************/
/* **************************
ASSERTS AND PRINTING
****************************/
#define PRINT_WIDTH 40

#define ASSERT(cond) do { \
		if (!(cond)) { \
			if (forked) { \
				P_CLEANUP(); \
				forked = FALSE; \
			} \
			if (cloned) { \
				T_CLEANUP(); \
				cloned = FALSE; \
			} \
			if (installed) \
				destroy_snake(); \
			sprintf(output, "FAILED, line %d: condition " #cond " is false",__LINE__); \
			return 0; \
		} \
	} while(0)

#define RUN_TEST(test) do { \
		int i,n = strlen(#test); \
		printf(#test); \
		for (i=0; i<PRINT_WIDTH-n-2; ++i) \
			printf("."); \
		if (HW4_TEST_DEBUG) \
			printf("\n"); \
		if (!(test())) \
			printf("%s",output); \
		else \
			printf("OK"); \
		printf(" \b\n"); \
	} while(0)
 
#define START_TESTS() do { \
		int i; \
		for(i=0; i<PRINT_WIDTH; ++i) printf("="); \
		printf("\nSTART TESTS\n"); \
		for(i=0; i<PRINT_WIDTH; ++i) printf("="); \
		printf("\n"); \
	} while(0)
 
#define END_TESTS() do { \
		int i; \
		for(i=0; i<PRINT_WIDTH; ++i) printf("="); \
		printf("\nDONE\n"); \
		for(i=0; i<PRINT_WIDTH; ++i) printf("="); \
		printf("\n"); \
	} while(0)

#define TEST_AREA(name) do { \
		int i; \
		for(i=0; i<PRINT_WIDTH; ++i) printf("-"); \
		printf("\nTesting "name" tests:\n"); \
		for(i=0; i<PRINT_WIDTH; ++i) printf("-"); \
		printf("\n"); \
	} while(0)

// Call this to update the percentage of completion of a test.
// Some tests may be long (stress testers) so this is useful.
// When we reach 100% I expect to output OK or FAILURE, so no
// need to output 100%... that's why I'm allowing 2 digits
#define UPDATE_PROG(percent) do { \
		printf("%2d%%\b\b\b",percent); \
	} while(0)

#define PROG_SET(total) do { \
		total_progress_indicators = total; \
		total_progress = 0; \
	} while(0)

#define PROG_BUMP() do { \
		UPDATE_PROG((total_progress++)*100/total_progress_indicators); \
	} while(0)

/* **************************************
PROCESS / THREAD HANDLING
****************************************/

// Creates n processes. Starts by creating an array to store the IDs
// and an integer to store which child you are.
#define FORK(n) do { \
		pids = (int*)malloc(sizeof(int)*n); \
		child_num=0; \
		if (n>0) forked = TRUE; \
		int i; \
		for (i=0; i<n; ++i) { \
			pids[i] = fork(); \
			if (!pids[i]) { \
				child_num = i+1; \
				break; \
			} \
		} \
	} while(0)

// Creates n clones. IDs (NOT INTEGERS!) are stored in tids[], and the function + parameter
// sent to all threads is the same.
#define CLONE(n,func,arg) \
	tids = (pthread_t*)malloc(sizeof(pthread_t)*(n)); \
	total_threads = n; \
	if (n>0) cloned = TRUE; \
	do { \
		int i; \
		for(i=0; i<total_threads; ++i) \
			ASSERT(!pthread_create(tids + i, NULL, func, (void*)(arg))); \
	} while(0)

// Returns TRUE if the process is the father (uses child_num)
#define P_IS_FATHER() ((bool)(!child_num))

// Waits for all children
#define P_WAIT() do { \
		int status; \
		while(wait(&status) != -1); \
	} while(0)

// Stops all processes except the father, who waits for them.
#define P_CLEANUP() do { \
		if (P_IS_FATHER()) P_WAIT(); \
		else exit(0); \
		free(pids); \
		child_num = -1; \
	} while(0)

// Used by the main thread, to join (wait) for the clones.
// Uses the tids[] array and the total_threads variable defined in CLONE()
#define T_WAIT() do { \
		int i; \
		for (i=0; i<total_threads; ++i) \
			pthread_join(tids[i],NULL); \
	} while(0)
			
#define T_CLEANUP() do { \
		int i; \
		T_WAIT(); \
		for (i=0; i<total_semaphores; ++i) \
			sem_destroy(tp.sem_arr + i); \
		free(tids); \
		free(tp.sem_arr); \
		total_threads = -1; \
		total_semaphores = -1; \
	} while(0)

#define SETUP_P(games,procs) \
	int fd; \
	setup_snake(games); \
	FORK(procs)

#define DESTROY_P() \
	P_CLEANUP(); \
	destroy_snake();

#define SETUP_T(games,threads,func,total_sems,sem_values) \
	tp.sem_arr = (sem_t*)malloc(sizeof(sem_t)*(total_sems)); \
	total_semaphores = total_sems; \
	do { \
		int i; \
		for(i=0; i<total_sems; ++i) \
			sem_init(tp.sem_arr + i, 0, sem_values[i]); \
	} while(0); \
	setup_snake(games); \
	CLONE(threads,func,(void*)&tp)

#define DESTROY_T() \
	T_CLEANUP(); \
	destroy_snake();

/* **************************************
SETUP
****************************************/

// Installs the module with max_games = n
void setup_snake(int n) {
	char n_char[4];
	sprintf(n_char, "%d", n);
	char *argv[] = { INSTALL_SCRIPT, n_char, '\0'};
	if (!fork()) {
		execv(INSTALL_SCRIPT, argv);
		exit(0);
	}
	else {
		P_WAIT();
		installed = TRUE;
	}
}

// Uninstalls the module
void destroy_snake() {
	if (!fork()) {
		execv(UNINSTALL_SCRIPT, '\0');
		exit(0);
	}
	else {
		P_WAIT();
		installed = FALSE;
	}
}

/* **************************************
GENERAL UTILITY
****************************************/

// Shuffles the contents of the given array.
void shuffle_arr(int* arr, int n) {
	int i;
	for (i=0; i<n; ++i) {
		int j = i + rand()%(n-i);
		int tmp = arr[i];
		arr[i]=arr[j];
		arr[j]=tmp;
	}
}

