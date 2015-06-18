#include "snake.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <asm/errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifdef VERBOSE
#define print(format, args...) \
	printf((format), ## args);
#else
#define print(format, args...)
#endif /* VERBOSE */

extern int errno;

#define DOWN	'2'
#define LEFT	'4'
#define RIGHT	'6'
#define UP		'8'

#define WHITE	4
#define BLACK	2

/**
 * Evaluates b and continues if b is true.
 * If b is false, ends the test by returning false and prints a detailed
 * message about the failure.
 */
#define ASSERT(b) do { \
        if (!(b)) { \
                printf("\nAssertion failed at %s:%d %s ",__FILE__,__LINE__,#b); \
                return false; \
        } \
} while (0)

/**
 * Macro used for running a test from the main function
 */
#define RUN_TEST(test) do { \
        printf("Running "#test"... "); \
        if (test) { \
            printf("[OK]\n");\
        } else { \
        	printf("[Failed]\n"); \
        	return 1; \
        } \
} while(0)

static void work();
static void workHard();
static bool testOpen();
static bool testRelease();
static bool testWrite1();
static bool testWrite2();
static bool testIlseek();
static bool testRead();
static bool testGameFlow();


static void work() {
	int i;
	for (i = 0; i < 1000; ++i) {
		short s;
		for (s = 1; s != 0; ++s)
			;
	}
}

static void workHard() {
	int i;
	for (i = 0; i < 50; ++i) {
		work();
	}
}

static bool testOpen() {
	if (fork() == 0) {
		int fd = open("/dev/snake0", O_RDWR);
		//print("fd = %d, errno = %d\n", fd, errno);
		ASSERT(fd >= 0);
		close(fd);
		_exit(0);
	} else {
		int fd = open("/dev/snake0", O_RDWR);
		//print("fd = %d, errno = %d\n", fd, errno);
		ASSERT(fd >= 0);
		close(fd);
		wait(0);
		return true;
	}
}

// If a player performed release, then the second wins.
static bool testRelease() {
	if (fork() == 0) {
		int fd = open("/dev/snake1", O_RDWR);
		ASSERT(fd >= 0);
		close(fd);
		_exit(0);
	} else {
		int fd = open("/dev/snake1", O_RDWR);
		ASSERT(fd >= 0);
		workHard();
		int color = ioctl(fd, SNAKE_GET_COLOR);
		int winner = ioctl(fd, SNAKE_GET_WINNER);
		ASSERT(winner == color);
		close(fd);
		wait(0);
		return true;
	}
}


//checking that wrong move makes you loose
static bool testWrite1() {
	if (fork() == 0) {
		int fd = open("/dev/snake3", O_RDWR);
		ASSERT(fd >= 0);
		char move = '1';
		write(fd, &move, 1);
		workHard();
		close(fd);
		_exit(0);
	} else {
		int fd = open("/dev/snake3", O_RDWR);
		ASSERT(fd >= 0);
		workHard();
		int color = ioctl(fd, SNAKE_GET_COLOR);
		int winner = ioctl(fd, SNAKE_GET_WINNER);
		printf("winner = %d color = %d\n", color, winner);
		ASSERT(winner == color);
		close(fd);
		wait(0);
		return true;
	}
}

//checking count = 0  and correct move is ok
static bool testWrite2() {
	if (fork() == 0) {
		int fd = open("/dev/snake4", O_RDWR);
		ASSERT(fd >= 0);
		char move = '2';
		write(fd, &move, 0);
		write(fd, &move, 1);
		workHard();
		close(fd);
		_exit(0);
	} else {
		int fd = open("/dev/snake4", O_RDWR);
		ASSERT(fd >= 0);
		char move = '6';
		write(fd, &move, 0);
		write(fd, &move, 1);
		workHard();
		int winner = ioctl(fd, SNAKE_GET_WINNER);
		ASSERT(winner != WHITE && winner != BLACK);
		close(fd);
		wait(0);
		return true;
	}
}

static bool testIlseek() {
	ASSERT(!llseek(getpid(), 1, 1));
	return true;
}


static bool testRead() {
		if (fork() == 0) {
		//THIS IS THE WHITE PLAYER
		int fd = open("/dev/snake5", O_RDWR);
		ASSERT(fd >= 0);
		char* buf = malloc(sizeof(char)*8);
		for(int i=0; i<8; i++){
			buf[i]='s';
		}
		ASSERT(read(fd, buf, 8));
		for(int i=0; i<8; i++){
			printf("i=%d and buf[i] = %c\n",i,buf[i]);
			ASSERT(buf[i] != 's');
		}
		close(fd);
		_exit(0);
	} else {
		int fd = open("/dev/snake5", O_RDWR);
		ASSERT(fd >= 0);
		workHard();
		close(fd);
		wait(0);
		return true;
	}
}

static bool testGameFlow() {
	if (fork() == 0) {
		//THIS IS THE WHITE PLAYER
		int fd = open("/dev/snake6", O_RDWR);
		ASSERT(fd >= 0);
		char move[2] = {DOWN,DOWN};
		write(fd, move, 2);
		workHard();
		workHard();
		close(fd);
		_exit(0);
	} else {
		int fd = open("/dev/snake6", O_RDWR);
		ASSERT(fd >= 0);
		
		char move = UP;
		write(fd, &move, 1);
		workHard();
		int winner = ioctl(fd, SNAKE_GET_WINNER);
		ASSERT(winner == BLACK);
		close(fd);
		wait(0);
		return true;
	}
	
}

int main() {
	//RUN_TEST(testOpen());
	//RUN_TEST(testRelease());
	RUN_TEST(testWrite1());
	RUN_TEST(testWrite2());
	//RUN_TEST(testIlseek());
	RUN_TEST(testRead());
	RUN_TEST(testGameFlow());
	return 0;
}

