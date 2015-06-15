#include "test_snake.h"

/*******************************************************************************************
 ===========================================================================================
 ===========================================================================================
                                   TEST FUNCTIONS
 ===========================================================================================
 ===========================================================================================
 ******************************************************************************************/
/* ***************************
 OPEN/RELEASE TESTS
*****************************/

// Test simple open and release.
// Make sure two processes do it, otherwise one will be stuck forever...
// calling open() blocks the process
bool open_release_simple() {
	SETUP_P(1,1);
	switch(child_num) {
	case 0:
	case 1:
		fd = open(get_node_name(0),O_RDWR);
		//printf("fd = %d", fd);
		ASSERT(fd >= 0);
		ASSERT(!close(fd));
	}
	DESTROY_P();
	return TRUE;
}

// Test failure of two releases (even after one open)
bool two_releases_processes() {
	SETUP_P(1,1);
	switch(child_num) {
	case 0:
	case 1:
		fd = open(get_node_name(0),O_RDWR);
		ASSERT(fd >= 0);
		ASSERT(!close(fd));
		ASSERT(close(fd));
	}
	DESTROY_P();
	return TRUE;
}

// Same thing, with threads
void* two_releases_func(void* arg) {
	ThreadParam *tp = (ThreadParam*)arg;
	int fd = open(get_node_name(0),O_RDWR);
	if(fd < 0) sem_post(tp->sem_arr);				// Did I fail to open?
	else if(!close(fd)) sem_post(tp->sem_arr+1);	// If not, did I fail to close?
	else if(close(fd)) sem_post(tp->sem_arr+2);		// If not, can I close again?
	return NULL;
}
bool two_releases_threads() {
	// sem1 - how many threads failed to open?
	// sem2 - how many threads failed to close once?
	// sem3 - how many threads succeeded in closing twice?
	int values[] = {0,0,0};
	SETUP_T(1,2,two_releases_func,3,values);
	int val;
	sem_getvalue(tp.sem_arr,&val);
	ASSERT(val == 0);
	sem_getvalue(tp.sem_arr+1,&val);
	ASSERT(val == 0);
	sem_getvalue(tp.sem_arr+2,&val);
	ASSERT(val == 0);
	DESTROY_T();
	return TRUE;
}

// Test failure of open-release-open (can't re-open a game!)
bool open_release_open() {
	SETUP_P(1,1);
	switch(child_num) {
	case 0:
	case 1:
		fd = open(get_node_name(0),O_RDWR);
		ASSERT(fd >= 0);
		ASSERT(!close(fd));
		fd = open(get_node_name(0),O_RDWR);
		ASSERT(fd < 0);
	}
	DESTROY_P();
	return TRUE;
}

// Make sure the first one is the white player
bool first_open_is_white() {
	
	// This may sometimes succeed, so try this many times
	//PRINT("in first_open_is_white()\n");
	int i, trials = 50;
	for (i=0; i<trials; ++i) {
		int res;
		//printf("in loop i = %d\n",i);
		UPDATE_PROG(i*100/trials);
		
		SETUP_P(1,1);
		switch(child_num) {
		case 0:
			//printf("case 0\n");
			fd = open(get_node_name(0),O_RDWR);
			//printf("case 0 open succeeded\n");
			ASSERT(fd >= 0);
			res = ioctl(fd,SNAKE_GET_COLOR,NULL);
			//printf("case 0 ioctl res = %d\n", res);
			ASSERT(res == WHITE_COLOR);
			//printf("case 0 after ioctl\n");
			close(fd);
			//printf("end of case 0\n");
			break;
		case 1:
			//printf("case 1\n");
			usleep(10000);	// 10ms, should be enough for father to be first to open
			//printf("case 1 after usleep\n");
			fd = open(get_node_name(0),O_RDWR);
			//printf("case 1 open succeeded\n");
			ASSERT(fd >= 0);
			usleep(10000);	// To prevent closing of a game before the white player can read the color
			PRINT("Son closing\n");
			close(fd);
			break;
		}
		DESTROY_P();
	}
	
	return TRUE;
}

// Test race - create 10 threads to try to open the same game, and make sure only two
// succeed each time.
// Do that T_OPEN_RACE_TRIES times (so if there is a deadlock situation, we might catch it).
// Uses open_race_thread_func and a ThreadParam passed to it to keep track of what's
// going on.
#define T_OPEN_RACE_TRIES 200
void* open_race_thread_func(void* arg) {
	ThreadParam *tp = (ThreadParam*)arg;
	int file_d = open(get_node_name(0),O_RDWR);
	if (file_d >= 0)
		sem_wait(tp->sem_arr);
	sem_post(tp->sem_arr+1);	// Signal father
	sem_wait(tp->sem_arr+2);	// Wait for father
	if (file_d >= 0)			// If I've open()ed, now I need to close()
		close(file_d);
	return NULL;
}
bool open_race_threads() {
	int i;
	for (i=0; i<T_OPEN_RACE_TRIES; ++i) {
		UPDATE_PROG(i*100/T_OPEN_RACE_TRIES);
		
		// Let 10 threads race (try to open() the same node)
		int threads = 10;
		int values[] = {threads,0,0};
		
		// Each thread that successfully open()s should lock sem1.
		// The father thread should lock sem2 #threads times, and each thread
		// Should signal this once. That way the father thread has an indication
		// of when it should keep running.
		// The father thread should signal sem3 #threads times to tell the clones
		// they can continue.
		// sem2 and sem3 are like a barrier.
		SETUP_T(1,threads,open_race_thread_func,3,values);
		
		int i;
		for (i=0; i<threads; ++i)			// Wait for clones
			sem_wait(tp.sem_arr+1);
		for (i=0; i<threads; ++i)			// Signal clones
			sem_post(tp.sem_arr+2);
		
		// Make sure exactly two succeeded
		int val;
		sem_getvalue(tp.sem_arr,&val);
		ASSERT(val == threads-2);		// Only 2 out of all threads should succeed in locking
		DESTROY_T();
	}
	return TRUE;
}

// Test the same thing, only with processes (forks)
#define P_OPEN_RACE_TRIES 200
bool open_race_processes() {
	int i;
	for (i=0; i<P_OPEN_RACE_TRIES; ++i) {
		UPDATE_PROG(i*100/P_OPEN_RACE_TRIES);
		SETUP_P(1,10);
		if (child_num) {	// All 10 children should try to open files
			fd = open("/dev/snake0",O_RDWR);
			fd < 0 ? exit(1) : exit(0);	// Send the parent success status
		}
		else {				// The parent should read the exit status
			int status;
			char opened, failed;
			opened = failed = 0;
			while(wait(&status) != -1) {
				WEXITSTATUS(status) ? ++failed : ++opened;
			}
			ASSERT(failed == 8);
			ASSERT(opened == 2);
		}
		DESTROY_P();
	}
	return TRUE;
}

// Stress test: create GAMES_RACE_T_NUM_GAMES games and GAMES_RACE_T_NUM_THREADS threads,
// each randomly trying to join games until all games are full.
// IMPORTANT: GAMES_RACE_T_NUM_GAMES cannot be greater than 25, because the maximum number of threads in linux 2.4
// is 256, so if you set GAMES_RACE_T_NUM_GAMES=26 the function will try to create 26*10=260>256 threads.
#define GAMES_RACE_NUM_TRIES 50
#define GAMES_RACE_T_NUM_GAMES 25
#define GAMES_RACE_T_NUM_THREADS (10*GAMES_RACE_T_NUM_GAMES)
void* games_race_func(void* arg) {
	
	ThreadParam *p = (ThreadParam*)arg;
	
	// Randomize the order of the games
	int game_numbers[GAMES_RACE_T_NUM_GAMES];
	int i;
	for(i=0; i<GAMES_RACE_T_NUM_GAMES; ++i)
		game_numbers[i] = i;
	shuffle_arr(game_numbers,GAMES_RACE_T_NUM_GAMES);
	
	// Try to open a game!
	// First game successfully opened, update it's semaphore and return.
	// If no game opened, update the last semaphore and return.
	for(i=0; i<GAMES_RACE_T_NUM_GAMES; ++i) {
		int file_d = open(get_node_name(game_numbers[i]),O_RDWR);
		if (file_d >= 0) {
			PROG_BUMP();
			sem_post(p->sem_arr+game_numbers[i]);
			close(file_d);
			return NULL;
		}
	}
	PROG_BUMP();
	sem_post(p->sem_arr+GAMES_RACE_T_NUM_GAMES);
	return NULL;
}
bool games_race_threads() {
	
	// Progress is updated every semaphore lock.
	// Each thread locks exactly one, so...
	PROG_SET(GAMES_RACE_T_NUM_THREADS*GAMES_RACE_NUM_TRIES);
	
	int j;
	for(j=0; j<GAMES_RACE_NUM_TRIES; ++j) {
		
		// GAMES_RACE_T_NUM_GAMES semaphores indicate how many threads opened a game.
		// The last semaphore indicates how many failed.
		int values[GAMES_RACE_T_NUM_GAMES+1] = { 0 };
		SETUP_T(GAMES_RACE_T_NUM_GAMES, GAMES_RACE_T_NUM_THREADS, games_race_func, GAMES_RACE_T_NUM_GAMES+1, values);
		T_WAIT();
		int val, i;
		for (i=0; i<GAMES_RACE_T_NUM_GAMES; ++i) {
			sem_getvalue(tp.sem_arr+i,&val);
			ASSERT(val == 2);
		}
		sem_getvalue(tp.sem_arr+GAMES_RACE_T_NUM_GAMES,&val);
		ASSERT(val == GAMES_RACE_T_NUM_THREADS-2*GAMES_RACE_T_NUM_GAMES);
		DESTROY_T();
	}
	
	return TRUE;
}

// Same thing, but with processes
#define GAMES_RACE_T_NUM_PROCS  (10*GAMES_RACE_T_NUM_GAMES)
bool games_race_processes() {
	
	
	int j;
	for(j=0; j<GAMES_RACE_NUM_TRIES; ++j) {
		
		UPDATE_PROG(j*100/GAMES_RACE_NUM_TRIES);
		
		SETUP_P(GAMES_RACE_T_NUM_GAMES,GAMES_RACE_T_NUM_PROCS);
		if (child_num) {
		
			// Randomize the order of the games
			int game_numbers[GAMES_RACE_T_NUM_GAMES];
			int i;
			for(i=0; i<GAMES_RACE_T_NUM_GAMES; ++i)
				game_numbers[i] = i;
			shuffle_arr(game_numbers,GAMES_RACE_T_NUM_GAMES);
			
			// Try to open a game!
			// First game successfully opened, exit() with it's index.
			// If no game opened, exit() with the last index.
			for(i=0; i<GAMES_RACE_T_NUM_GAMES; ++i) {
				int file_d = open(get_node_name(game_numbers[i]),O_RDWR);
				if (file_d >= 0) {
					close(file_d);
					exit(game_numbers[i]);
				}
			}
			exit(GAMES_RACE_T_NUM_GAMES);
		}
		else {
			int status, results[GAMES_RACE_T_NUM_GAMES+1] = { 0 };
			while(wait(&status) != -1) {
				results[WEXITSTATUS(status)]++;
			}
			int i;
			for (i=0; i<GAMES_RACE_T_NUM_GAMES; ++i)
				ASSERT(results[i] == 2);
			ASSERT(results[GAMES_RACE_T_NUM_GAMES] == GAMES_RACE_T_NUM_PROCS-2*GAMES_RACE_T_NUM_GAMES);
		}
		DESTROY_P();
	}
	return TRUE;
}



/* ***************************
 GENERAL TESTS
*****************************/
// All functions should return -10 (-1 returned, errno=10) after one process releases
bool error_after_release() {
	
	return TRUE;
}


/*******************************************************************************************
 ===========================================================================================
 ===========================================================================================
										MAIN
 ===========================================================================================
 ===========================================================================================
 ******************************************************************************************/

int main() {
	
	// Prevent output buffering, so we don't see weird shit when multiple processes are active
	setbuf(stdout, NULL);
	
	// Seed the random number generator
	srand(time(NULL));
	
	// Test!
	START_TESTS();
	
	TEST_AREA("open & release");
	RUN_TEST(open_release_simple);
	RUN_TEST(two_releases_processes);
	RUN_TEST(two_releases_threads);
	RUN_TEST(open_release_open);
	RUN_TEST(first_open_is_white);
	RUN_TEST(open_race_threads);
	RUN_TEST(open_race_processes);
	RUN_TEST(games_race_threads);
	RUN_TEST(games_race_processes);
	
	// That's all folks
	END_TESTS();
	return 0;
	
}

