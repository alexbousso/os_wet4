/*
 * test_module.c
 *
 *  Created on: 7 áéðå 2015
 *      Author: alon
 */
#include <asm/errno.h>
extern int errno;
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "snake.h"
#include <sys/wait.h>

/*========================[instructions]========================*/
	// compile this test with:
	// gcc -Wall test_snake.c -o test_snake
	// then run with:
	// ./test_snake



	//here is an example of a driver install script:

	// #!/bin/bash
	// rm /dev/snake0
	// rm /dev/snake1
	// rm /dev/snake2
	// rm /dev/snake3
	//	.
	//	.
	//	.
	//	.
	// rm /dev/snake50
	// rmmod snake

	// make 
	// insmod ./snake.o max_games=50
	// mknod /dev/snake0 c 254 0
	// mknod /dev/snake1 c 254 1
	// mknod /dev/snake2 c 254 2
	// mknod /dev/snake3 c 254 3
	//	.
	//	.
	//	.
	//	.
	//	.
	// mknod /dev/snake50 c 254 50
/*========================[instructions]========================*/


/*========================[DEFINES]========================*/
	#define DOWN  '2'
	#define LEFT  '4'
	#define RIGHT '6'
	#define UP    '8'
	#define BUFFER_SIZE 1024


	#define ASSERT(b) do { \
	    if (!(b)) { \
	            printf("\nAssertion failed at %s:%d %s ",__FILE__,__LINE__,#b); \
	            return false; \
	    } \
	} while (0)


	#define RUN_TEST(test) do { \
	    printf("Running "#test"... "); \
	    if (test()) { \
	        printf("[OK]\n");\
	    } else { \
	            printf("[Failed]\n"); \
	    } \
	} while(0)
		
	#define ASSERT_INT_EQUALS(res,exp) do { \
		if ((exp) != (res)) { \
			printf("\nExpected: %d, Response: %d", (exp), (res)); \
		} \
		ASSERT((exp) == (res)); \
	} while(0)
/*========================[DEFINES]========================*/


/*========================[UTILS]========================*/
	void doMediumTask()
	{
	   int j;
	   for(j=0; j<1000; j++)
	   {
	      short i;
	      for (i=1; i != 0; i++)
	      {
	         ;
	      }
	   }
	}


	void doLongTask()
	{
	   int j;
	   for(j=0; j<30000; j++)
	   {
	      short i;
	      for (i=1; i != 0; i++)
	      {
	         ;
	      }
	   }
	}

	char moduleName[13];
	char* moduleBase = "/dev/snake";
	int moduleNum = 0;

	static int isGameBoy = 0;
	static int *numOfPlayers;
	static int *gameHasEnded;
	static char *nextMove;
	static int *player;
	static int *Currentplayer;
	static int *a;
	static int *b;
	int doNotPrint = 0;
	char* getModule()
	{
		int i;
		for (i = 0; i < 13; ++i) { moduleName[i] = '\0'; }
				
		char moduleIndex[4]; 
		for (i = 0; i < 4; ++i) { moduleIndex[i] = '\0'; }
			
		sprintf(moduleIndex, "%d", moduleNum); //convert int to string

		strcat(moduleName , moduleBase );	
		strcat(moduleName , moduleIndex );
		moduleNum++;

		if (doNotPrint==0)
		{
			printf("Entering driver: %s\n",moduleName);
		}
		return moduleName;
	}


	void cleanScreen()
	{
		printf("\033[2J");
		printf("\r"); 
	}

	void printLogo()
	{
	  	char* gameboyLogo = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |   _______ **** __  |  |\n|  |  /       \\ ** /  \\ |  |\n|  | /  /~~~\\  \\  (o  o)|  |\n|  | |  |   |  |   |  | |  |\n|  |(o  o)  \\  \\___/  / |  |\n|  | \\__/ ** \\       /  |  |\n|  |  |  **** ~~~~~~~   |  |\n|  |  ^                 |  |\n|  |       LOADING      |  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _| |_         ,-.     |\n|   |_ O _|   ,-. \"._,\"    |\n|     |_|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"\n";
		cleanScreen(); 
		printf("\r");   	
		printf("%s\n",gameboyLogo);               
	}

	void printDeadSnake()
	{
	  	char* deadSnake = "|      _______ **** __     |\n|     /       \\ ** /  \\    |\n|    /  /~~~\\  \\  (X  X)   |\n|    |  |   |  |   |  |    |\n|   (X  X)  \\  \\___/  /    |\n|    \\__/ ** \\       /     |\n|     |  **** ~~~~~~~      |\n|     ^                    |\n|   * SNAKES  CRASHED! *   |\n";
		printf("%s\n",deadSnake);   		
	}


	void printGameOver()
	{
	  	char* gameboyLogo = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |   _______ **** __  |  |\n|  |  /       \\ ** /  \\ |  |\n|  | /  /~~~\\  \\  (o  o)|  |\n|  | |  |   |  |   |  | |  |\n|  |(X  X)  \\  \\___/  / |  |\n|  | \\__/ ** \\       /  |  |\n|  |  |  **** ~~~~~~~   |  |\n|  |  ^                 |  |\n|  |***** GAME OVER ****|  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _| |_         ,-.     |\n|   |_ O _|   ,-. \"._,\"    |\n|     |_|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"\n";
		cleanScreen(); 
		printf("\r");   	
		printf("%s\n",gameboyLogo);      
	}

	void printBoard(char* board)
	{
		if (isGameBoy == 0)
		{
			printf("-------[Board]-------\n");
			printf("%s\n", board);
			return;
		}

		cleanScreen(); 
		printf("\r"); 
		char* upper = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |********************|  |\n|  |********************|  |";
		char* upperWhite = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |********WHITE*******|  |\n|  |********************|  |";
		char* upperBlack = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |********BLACK*******|  |\n|  |********************|  |";

		if (*Currentplayer == 1)
		{
			printf("%s\n",upperBlack);
		}
		else if (*Currentplayer == -1)
		{
			printf("%s\n",upperWhite);
		}
		else if (*Currentplayer == 0)
		{
			printf("%s\n",upper);
		}


		char middle[BUFFER_SIZE];
		int i;
		for (i = 0; i < BUFFER_SIZE; ++i)
		{
			middle[i] = '\0';
		}
		//row1
		strcat(middle,"|  |**");
		strcat(middle,board);
		middle[21] = '\0';
		strcat(middle,"***|  |\n");

		//row2
		strcat(middle,"|  |**");
		strcat(middle,board+16);
		middle[50] = '\0';
		strcat(middle,"***|  |\n");

		//row3
		strcat(middle,"|  |**");
		strcat(middle,board+32);
		middle[79] = '\0';
		strcat(middle,"***|  |\n");

		//row4
		strcat(middle,"|  |**");
		strcat(middle,board+48);
		middle[108] = '\0';
		strcat(middle,"***|  |\n");

		//row5
		strcat(middle,"|  |**");
		strcat(middle,board+64);
		middle[137] = '\0';
		strcat(middle,"***|  |\n");

		//row6
		strcat(middle,"|  |**");
		strcat(middle,board);
		middle[166] = '\0';
		strcat(middle,"***|  |\0");

		printf("%s\n",middle );

		char* downer = "|  |********************|  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _| |_         ,-.     |\n|   |_ O _|   ,-. \"._,\"    |\n|     |_|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"\n";
		printf("%s\n",downer);
	}

	void printButtonPress(char* board , char* dir)
	{
		char direction = dir[0];
		cleanScreen(); 
		char* upperWhite = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |********WHITE*******|  |\n|  |********************|  |";
		char* upperBlack = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |********BLACK*******|  |\n|  |********************|  |";

		if (*Currentplayer == 1)
		{
			printf("%s\n",upperBlack);
		}
		else if (*Currentplayer == -1)
		{
			printf("%s\n",upperWhite);
		}


		char middle[BUFFER_SIZE];
		int i;
		for (i = 0; i < BUFFER_SIZE; ++i)
		{
			middle[i] = '\0';
		}
		//row1
		strcat(middle,"|  |**");
		strcat(middle,board);
		middle[21] = '\0';
		strcat(middle,"***|  |\n");

		//row2
		strcat(middle,"|  |**");
		strcat(middle,board+16);
		middle[50] = '\0';
		strcat(middle,"***|  |\n");

		//row3
		strcat(middle,"|  |**");
		strcat(middle,board+32);
		middle[79] = '\0';
		strcat(middle,"***|  |\n");

		//row4
		strcat(middle,"|  |**");
		strcat(middle,board+48);
		middle[108] = '\0';
		strcat(middle,"***|  |\n");

		//row5
		strcat(middle,"|  |**");
		strcat(middle,board+64);
		middle[137] = '\0';
		strcat(middle,"***|  |\n");

		//row6
		strcat(middle,"|  |**");
		strcat(middle,board);
		middle[166] = '\0';
		strcat(middle,"***|  |\0");

		printf("%s\n",middle );

		char* downerUp = "|  |********************|  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _|X|_         ,-.     |\n|   |_ O _|   ,-. \"._,\"    |\n|     |_|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"";
		char* downerDown = "|  |********************|  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _| |_         ,-.     |\n|   |_ O _|   ,-. \"._,\"    |\n|     |X|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"";
		char* downerRight = "|  |********************|  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _| |_         ,-.     |\n|   |_ O X|   ,-. \"._,\"    |\n|     |_|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"";
		char* downerLeft = "|  |********************|  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _| |_         ,-.     |\n|   |X O _|   ,-. \"._,\"    |\n|     |_|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"";
		
		if (direction == UP)
		{
			printf("%s\n",downerUp);
		}
		else if (direction == RIGHT)
		{
			printf("%s\n",downerRight);
		}
		else if (direction == LEFT)
		{
			printf("%s\n",downerLeft);	
		}
		else if (direction == DOWN)
		{
			printf("%s\n",downerDown);	
		}
		
		doMediumTask();
		doMediumTask();
		doMediumTask();
	}

	void printingProgressBar()
	{
		char* loading1 = "\n**********************\n**********************\n**********************\n**------------------**\n**-[GAME OF SNAKES]-**\n**------------------**\n*******[LOADING]******\n**********************\n[                    ]\n";
		char* loading2 = "\n**********************\n**********************\n**********************\n**------------------**\n**-[GAME OF SNAKES]-**\n**------------------**\n*******[LOADING]******\n**********************\n[+++++               ]\n";
		char* loading3 = "\n**********************\n**********************\n**********************\n**------------------**\n**-[GAME OF SNAKES]-**\n**------------------**\n*******[LOADING]******\n**********************\n[++++++++++          ]\n";
		char* loading4 = "\n**********************\n**********************\n**********************\n**------------------**\n**-[GAME OF SNAKES]-**\n**------------------**\n*******[LOADING]******\n**********************\n[+++++++++++++++     ]\n";
		char* loading5 = "\n**********************\n**********************\n**********************\n**------------------**\n**-[GAME OF SNAKES]-**\n**------------------**\n*******[LOADING]******\n**********************\n[++++++++++++++++++++]\n";

		char* loading[5] = {loading1,loading2,loading3,loading4,loading5};
		//print text
		int i;
		for (i = 0; i < 5; ++i)
		{
			int j;
			for (j = 0; j < 6; ++j)
			{
				doMediumTask();
			}
			printf("\033[11A");
			printf("\r");
			printf("%s\n",loading[i]);
		} 
	}

	void printWelcome()
	{
		char* gameboy1 = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |                    |  |\n|  |                    |  |\n|  |                    |  |\n|  |                    |  |\n|  |                    |  |\n|  |                    |  |\n|  |                    |  |\n|  |                    |  |\n|  |                    |  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _| |_         ,-.     |\n|   |_ O _|   ,-. \"._,\"    |\n|     |_|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"\n";
		char* gameboy2 = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _| |_         ,-.     |\n|   |_ O _|   ,-. \"._,\"    |\n|     |_|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"\n";
		char* gameboy3 = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |******[WELCOME]*****|  |\n|  |********[TO]********|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _| |_         ,-.     |\n|   |_ O _|   ,-. \"._,\"    |\n|     |_|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"\n";
		char* gameboy4 = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |* [GAME OF SNAKES] *|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _| |_         ,-.     |\n|   |_ O _|   ,-. \"._,\"    |\n|     |_|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"\n";
		char* gameboy5 = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |* [GAME OF SNAKES] *|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |*[PLAY]**[RUN TEST]*|  |\n|  |********************|  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _| |_         ,-.     |\n|   |_ O _|   ,-. \"._,\"    |\n|     |_|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"\n";

		char* gameboy[5] = {gameboy1,gameboy2,gameboy3,gameboy4,gameboy5};

		int i;
		for (i = 0; i < 5; i++)
		{
			cleanScreen(); 
			printf("%s\n", gameboy[i]);
			if (i==4)
			{
				printf("Press A to [PLAY], Press B to [RUN TEST]\n");
			}
			int j;
			for (j = 0; j < 6; j++){
				doMediumTask();
			}
		}
	}
/*========================[UTILS]========================*/




/*========================[TESTS]========================*/
bool CreateNewGameTest()
{
	char* moduleName = getModule(); //e.g "/dev/snake0"
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);
		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			doLongTask(); // to make sure im the second one
			//player 2 (black)
			b=open(moduleName, O_RDWR);
			ASSERT(b>=0);
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}
    return true;
}

bool CreateGameSameModuleAfterCancelTest()
{
	char* moduleName = getModule();
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);
		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			doLongTask(); // to make sure im the second one
			//player 2 (black)
			b=open(moduleName, O_RDWR);
			ASSERT(b>=0);
			close(b); 
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}
    close(a);
    close(b);

    // game was canceled, now we gonna try again
    doLongTask();

	int pid3 = fork();
	if(pid3 == 0) {
		//player 1 (white)
		a=open(moduleName, O_RDWR); //should fail
		ASSERT(a<0);
		doLongTask();
		_exit(0);
	} else {
		int pid4 = fork();
		if(pid4 == 0) {
			doLongTask(); // to make sure im the second one
			//player 2 (black)
			b=open(moduleName, O_RDWR);  //should fail
			ASSERT(b<0); 
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}
    close(a);
    close(b);	
    return true;
}

bool CreateGameWithThreePlayersTest()
{
	char* moduleName = getModule();
	int a;
	int b;
	int c;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			doLongTask(); // to make sure im the second one
			//player 2 (black)
			b=open(moduleName, O_RDWR);
			ASSERT(b>=0);
			doLongTask();
			doLongTask();
			doLongTask();
			doLongTask();
			close(b);
			_exit(0);
		} else {
			int pid3 = fork();
			if(pid3 == 0) {
				doLongTask();
				doLongTask();
				//player 3
				c=open(moduleName, O_RDWR);
				ASSERT(c<0); 
				_exit(0);
			} else {
				wait(&status);
			}
			wait(&status);
		}
		wait(&status);
	}	
    return true;
}

bool PrintBoardTest()
{
	char* moduleName = getModule();
	char board[BUFFER_SIZE+1];
	board[BUFFER_SIZE]='\0'; // <---- might be redundant but i cant leave it like that! it's a disturbia
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);


		int readRes = read(a, NULL, BUFFER_SIZE);
		ASSERT(readRes == -1); // <---- should bring 0 according to the metargelim (piazza return -EFAULT;)

		readRes = read(a, board, 0);
		ASSERT(readRes == 0);	// <---- should bring 0 according to the metargelim (piazza return 0;)

		readRes = read(a, NULL, 0);
		ASSERT(readRes == 0);	// <---- should bring 0 according to the metargelim (piazza return 0;)


		//LET PLAYER 2 THE CHANCE TO PRINT BEFORE I GO		
		int j;
		for (j = 0; j < 6; j++){
			doMediumTask();
		}

		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);
			int readRes = read(b, board, BUFFER_SIZE);
			printBoard(board);
			ASSERT(readRes == BUFFER_SIZE);
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}	
    return true;
}

bool PrintBoardSmallCountTest()
{
	char* moduleName = getModule();
	char board[BUFFER_SIZE+1];
	board[BUFFER_SIZE]='\0'; // <---- might be redundant but i cant leave it like that! it's a disturbia
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);



		//LET PLAYER 2 THE CHANCE TO PRINT BEFORE I GO		
		int j;
		for (j = 0; j < 6; j++){
			doMediumTask();
		}

		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);


			int readRes = read(b, board, 40);
			printBoard(board);
			ASSERT(readRes == 40);
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}	
    return true;
}

bool MyFirstMoveTest()
{
	char* moduleName = getModule();
	char board[BUFFER_SIZE+1];
	board[BUFFER_SIZE]='\0'; // <---- might be redundant but i cant leave it like that! it's a disturbia
	char nextMove = DOWN;
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		//board before
		int readRes = read(a, board, BUFFER_SIZE);	
		ASSERT(readRes == BUFFER_SIZE);
		printf("board before move is:\n");
		printBoard(board);

		int writeval = write(a, &nextMove, 1);	
		ASSERT(writeval == 1);

		//board after move
		readRes = read(a, board, BUFFER_SIZE);	
		ASSERT(readRes == BUFFER_SIZE);
		printf("board after move is:\n");
		printBoard(board);

		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			doLongTask();
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}	
    return true;
}

bool MakeMoveCrashWallTest()
{
	char* moduleName = getModule();
	char board[BUFFER_SIZE+1];
	board[BUFFER_SIZE]='\0'; // <---- might be redundant but i cant leave it like that! it's a disturbia
	char nextMove = UP;
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		//board before
		int readRes = read(a, board, BUFFER_SIZE);	
		ASSERT(readRes == BUFFER_SIZE);

		int writeval = write(a, &nextMove, 1);
		ASSERT_INT_EQUALS(writeval, 1); // <---- should be 1 according to the metargelim

		//board after illigal move
		readRes = read(a, board, BUFFER_SIZE);	
		ASSERT(readRes == BUFFER_SIZE); // <---- should success according to the metargelim even if the game over

		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			doLongTask();
			doLongTask();
			doLongTask();
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}	
    return true;
}

bool MakeMoveIlligalCharacterTest()
{
	char* moduleName = getModule();
	char board[BUFFER_SIZE+1];
	board[BUFFER_SIZE]='\0'; // <---- might be redundant but i cant leave it like that! it's a disturbia
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		char nextMove = DOWN;
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		int writeval = write(a, &nextMove, 1);	
		ASSERT(writeval == 1);

		//board after illigal move
		int readRes = read(a, board, BUFFER_SIZE);	
		ASSERT(readRes == BUFFER_SIZE); // <---- should success according to the metargelim even if the game over by the illigal char of black

		doLongTask();
		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			doLongTask();
			char nextMove = '9';
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			int writeval = write(b, &nextMove, 1);
			ASSERT_INT_EQUALS(writeval, -1); //<---- writing illigal char should return -1 according to the metargelim

			//board after illigal move
			int readRes = read(b, board, BUFFER_SIZE);	
			ASSERT(readRes == BUFFER_SIZE); // <---- should success according to the metargelim even if the game over by the illigal char of black


			doLongTask();
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}	
    return true;
}

bool MakeMultipleMovesTest()
{
	//you should see somthing like:
	// 3  2  1   (might also be with 4)
	//-3 -2 -1   (might also be with -4)
	char* moduleName = getModule();
	char board[BUFFER_SIZE+1];
	board[BUFFER_SIZE]='\0'; // <---- might be redundant but i cant leave it like that! it's a disturbia
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		char nextMove[5];
		nextMove[0] = DOWN; 
		nextMove[1] = RIGHT; 
		nextMove[2] = RIGHT; 
		nextMove[3] = RIGHT; 
		nextMove[4] = '\0'; 
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		int writeval = write(a, nextMove, 4);	
		ASSERT_INT_EQUALS(writeval, 4);

		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			char nextMove[5];
			nextMove[0] = UP;
			nextMove[1] = RIGHT; 
			nextMove[2] = RIGHT; 
			nextMove[3] = RIGHT; 
			nextMove[4] = '\0'; 

			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			int writeval = write(b, nextMove, 4);	
			ASSERT(writeval == 4);


			//board after move
			int readRes = read(b, board, BUFFER_SIZE);	
			ASSERT(readRes == BUFFER_SIZE);
			printBoard(board);

			doLongTask();
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}	
    return true;
}

bool MakeMoveCrashSnakeTest()
{
	char* moduleName = getModule();
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		char nextMove[4];
		nextMove[0] = DOWN; 
		nextMove[1] = DOWN; 
		nextMove[2] = DOWN; 
		nextMove[3] = '\0'; 
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		int writeval = write(a, nextMove, 3);
		ASSERT_INT_EQUALS(writeval, 2); // <---- im the one who crashes so i did 2 step successfully

		doLongTask();
		doLongTask();
		doLongTask();
		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			char nextMove[4];
			nextMove[0] = UP; 
			nextMove[1] = UP;   // <---- here game does already OVER
			nextMove[2] = UP; 
			nextMove[3] = '\0'; 
			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			int writeval = write(b, nextMove, 3);
			ASSERT(writeval == 1); // <---- we successfully made 1 move inside write, return 1 according to the metargelim

			printDeadSnake();

			doLongTask();
			doLongTask();
			doLongTask();
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}	
    return true;
}

bool MakeMultipleMovesWithIlligalMoveTest()
{
	char* moduleName = getModule();
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		char nextMove[6];
		nextMove[0] = DOWN; 
		nextMove[1] = RIGHT; 
		nextMove[2] = '9';    //   <-------illigal!
		nextMove[3] = RIGHT; 
		nextMove[4] = UP; 
		nextMove[5] = '\0'; 
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		int writeval = write(a, nextMove, 5);
		ASSERT_INT_EQUALS(writeval, -1); // <----- return -1 according to the metargelim for illigal move inside buffer

		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			char nextMove[4];
			nextMove[0] = UP;
			nextMove[1] = RIGHT; 
			nextMove[2] = RIGHT; 
			nextMove[3] = '\0'; 

			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			int writeval = write(b, nextMove, 3);
			ASSERT(writeval == 2); // <----- return 2 cause i did 2 moves succesfully, according to the metargelim

			doLongTask();
			close(b);
			_exit(0);
		} else {
			//waitpid(pid2, &status,WNOHANG);
			wait(&status);
		}

		//waitpid(pid1, &status,WNOHANG);
		wait(&status);
	}	
    return true;
}

bool SnakeGetColorTest()
{
	char* moduleName = getModule();
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		int ioctl_retval = ioctl(a, SNAKE_GET_COLOR);
        ASSERT(ioctl_retval == 4);

		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			int ioctl_retval = ioctl(b, SNAKE_GET_COLOR);
        	ASSERT(ioctl_retval == 2);

			doLongTask();
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}
    return true;
}

bool GetWinnerWhiteWinTest()
{
	char* moduleName = getModule();
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		char nextMove[5];
		nextMove[0] = DOWN; 
		nextMove[1] = RIGHT; 
		nextMove[2] = RIGHT; 
		nextMove[3] = RIGHT; 
		nextMove[4] = '\0'; 
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		int writeval = write(a, nextMove, 4);	
		ASSERT(writeval == 2);  // <----- return 2 cause i did 2 moves succesfully, according to the metargelim

		int ioctl_retval = ioctl(a, SNAKE_GET_WINNER);
        ASSERT(ioctl_retval == 4);

		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			char nextMove[5];
			nextMove[0] = UP;
			nextMove[1] = UP; // <----- will crash white
			nextMove[2] = UP; 
			nextMove[3] = UP; 
			nextMove[4] = '\0'; 

			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			int writeval = write(b, nextMove, 4);	
			ASSERT_INT_EQUALS(writeval, 2); // <----- return 2 cause i did 2 moves succesfully, according to the metargelim

			int ioctl_retval = ioctl(b, SNAKE_GET_WINNER);
	        ASSERT(ioctl_retval == 4); //even if im black i should know that the white won!

			doLongTask();
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}
	return true;		
}

bool GetWinnerBlackWinTest()
{
	char* moduleName = getModule();
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		char nextMove[5];
		nextMove[0] = DOWN; 
		nextMove[1] = DOWN; 
		nextMove[2] = DOWN; 
		nextMove[3] = DOWN; 
		nextMove[4] = '\0'; 
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		int writeval = write(a, nextMove, 4);	
		ASSERT(writeval == 2); // <----- return 2 cause i did 2 moves succesfully, according to the metargelim

		int ioctl_retval = ioctl(a, SNAKE_GET_WINNER);
        ASSERT(ioctl_retval == 2); // <------- even if im white i should know that the black won!

		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			char nextMove[5];
			nextMove[0] = UP;
			nextMove[1] = RIGHT; 
			nextMove[2] = RIGHT; 
			nextMove[3] = RIGHT; 
			nextMove[4] = '\0'; 

			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			int writeval = write(b, nextMove, 4);	
			ASSERT(writeval == 1);  // <----- return 1 cause i did 1 moves succesfully, according to the metargelim

			int ioctl_retval = ioctl(b, SNAKE_GET_WINNER);
	        ASSERT(ioctl_retval == 2); // <----- i won!!!

			doLongTask();
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}
	return true;	
}

bool GetWinnerGameInProgressTest()
{
	char* moduleName = getModule();
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		char nextMove = DOWN;
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		int writeval = write(a, &nextMove, 1);	
		ASSERT(writeval == 1);

		int ioctl_retval = ioctl(a, SNAKE_GET_WINNER);
        ASSERT(ioctl_retval == -1); // <----- game is not finished

		nextMove = RIGHT; 

		writeval = write(a, &nextMove, 1);	
		ASSERT(writeval == 1);


		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			char nextMove = UP;

			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			int writeval = write(b, &nextMove, 1);	
			ASSERT(writeval == 1);

			int ioctl_retval = ioctl(b, SNAKE_GET_WINNER);
	        ASSERT(ioctl_retval == -1); // <----- game is not finished

			nextMove = RIGHT;

	        writeval = write(b, &nextMove, 1);	
			ASSERT(writeval == 1);

			doLongTask();
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}
	return true;	
}

bool MakeSimpleGameGameAux(char* myModuleName)
{
	char* moduleName = myModuleName;
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		char nextMove = DOWN;
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		int writeval = write(a, &nextMove, 1);	
		ASSERT(writeval == 1);

		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			char nextMove = UP;
			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			int writeval = write(b, &nextMove, 1);	
			ASSERT(writeval == 1);

			doLongTask();
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}	
    return true;
}
bool TreeGamesAtTheSameTime()
{
	doNotPrint = 1;
	printf("This might take a while...\n");
	//some birocrat shit you can ignore
	char moduleName1[13]; moduleName1[0] = '\0';
	char moduleName2[13]; moduleName2[0] = '\0';
	char moduleName3[13]; moduleName3[0] = '\0';
	char* tempModule = getModule();
	strcpy(moduleName1, tempModule);
	tempModule = getModule();
	strcpy(moduleName2, tempModule);
	tempModule = getModule();
	strcpy(moduleName3, tempModule);
	doNotPrint = 0;

	//test start here
	static int *StartGames;
	StartGames = mmap(NULL, sizeof(*StartGames), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*StartGames = 0;

	int status;
	int game1 = fork();
	if(game1 == 0) 
	{
		//this will be game 1
		while(*StartGames==0)
		{
			//halt until everyone is ready
			;
		}
		printf("Im Entering driver: %s\n",moduleName1);
		MakeSimpleGameGameAux(moduleName1);
		_exit(0);
	}
	else
	{
			int game2 = fork();
			if(game2 == 0) 
			{
				//this will be game 2
				while(*StartGames==0)
				{
					//halt until everyone is ready
					;
				}
				printf("Im Entering driver: %s\n",moduleName2);
				MakeSimpleGameGameAux(moduleName2);
				_exit(0);
			}
			else
			{
					int game3 = fork();
					if(game3 == 0) 
					{
						//this will be game 3
						while(*StartGames==0)
						{
							//halt until everyone is ready
							;
						}
						printf("Im Entering driver: %s\n",moduleName3);
						MakeSimpleGameGameAux(moduleName3);
						_exit(0);
					}
					else
					{
							doLongTask();
							doLongTask();
							doLongTask();
							//Launch games!
							*StartGames=1;
						wait(&status);
					}
				wait(&status);
			}
		wait(&status);
	}
	munmap(StartGames, sizeof (*StartGames));
	return true;
}

bool TwoWhitesAgainstOneBlackTest()
{
	printf("\nno offend to black people, you guys are awesome!\n");
	char* moduleName = getModule();
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		int myBetterHalf = fork();
		if(myBetterHalf == 0)
		{
			char nextMove[5];
			nextMove[0] = DOWN; 
			nextMove[1] = RIGHT; 
			nextMove[2] = RIGHT; 
			nextMove[3] = RIGHT; 
			nextMove[4] = '\0'; 

			int writeval = write(a, nextMove, 4);	
			ASSERT(writeval == 2); // <------ i did 2 good moves, give me a break
			_exit(0);
		}
		else
		{
			doLongTask();
			char nextMove[5];
			nextMove[0] = LEFT; 
			nextMove[1] = LEFT; 
			nextMove[2] = LEFT; 
			nextMove[3] = LEFT; 
			nextMove[4] = '\0'; 
			int writeval = write(a, nextMove, 4);			
			ASSERT(writeval == -1); // <------ should be in the game after its already finished

			doLongTask();
			close(a);
			wait(&status);
		}
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			char nextMove = UP;

			doLongTask();
			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			int writeval = write(b, &nextMove, 1);	
			ASSERT(writeval == 1);

			doLongTask();
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}	
    return true;
}

bool ThreeAgainstThreeTest()
{
	char* moduleName = getModule();
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);

		int myFirstSon = fork();
		if(myFirstSon == 0)
		{
			char nextMove[5];
			nextMove[0] = DOWN; 
			nextMove[1] = RIGHT; 
			nextMove[2] = RIGHT; 
			nextMove[3] = RIGHT; 
			nextMove[4] = '\0'; 

			int writeval = write(a, nextMove, 4);	
			ASSERT_INT_EQUALS(writeval,4);
			_exit(0);
		}
		else
		{
			doLongTask();
			int mySecondSon = fork();
			if(mySecondSon == 0)
			{
				char nextMove[5];
				nextMove[0] = DOWN; 
				nextMove[1] = RIGHT; 
				nextMove[2] = RIGHT; 
				nextMove[3] = RIGHT; 
				nextMove[4] = '\0'; 

				int writeval = write(a, nextMove, 4);	
				ASSERT(writeval == -1);
				_exit(0);
			}
			else
			{
				doLongTask();
				doLongTask();
				char nextMove[5];
				nextMove[0] = DOWN; 
				nextMove[1] = RIGHT; 
				nextMove[2] = RIGHT; 
				nextMove[3] = RIGHT; 
				nextMove[4] = '\0'; 
				int writeval = write(a, nextMove, 4);
					
				ASSERT(writeval == -1);
				doLongTask();
				close(a);
				wait(&status);
			}
			wait(&status);
		}
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			//player 2 (black)
			b=open(moduleName, O_RDWR);
			ASSERT(b>=0);

			int myFirstSon = fork();
			if(myFirstSon == 0)
			{
				char nextMove = UP;

				int writeval = write(b, &nextMove, 1);	
				ASSERT(writeval == 1);
				_exit(0);
			}
			else
			{
				int mySecondSon = fork();
				if(mySecondSon == 0)
				{
					doLongTask();
					char nextMove = RIGHT;

					int writeval = write(b, &nextMove, 1);	
					ASSERT_INT_EQUALS(writeval,1);
					_exit(0);
				}
				else
				{
					//father code
					doLongTask();
					char nextMove = RIGHT;
					int writeval = write(b, &nextMove, 1);
						
					ASSERT(writeval == 1);
					doLongTask();
					close(b);
					wait(&status);
				}
				wait(&status);
			}
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}	
    return true;
}

bool AgressiveReadWriteTest()
{
	char* moduleName = getModule();
	int a;
	int b;
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		char nextMove = DOWN;
		char board[BUFFER_SIZE+1];
		board[BUFFER_SIZE]='\0'; // <---- might be redundant but i cant leave it like that! it's a disturbia
		//player 1 (white)
		a=open(moduleName, O_RDWR);
		ASSERT(a>=0);
		int i;
		for (i = 0; i < 500; ++i)
		{
			int readRes = read(a, board, BUFFER_SIZE);	
			ASSERT(readRes == BUFFER_SIZE);
		}

		int writeval = write(a, &nextMove, 1);	
		ASSERT(writeval == 1);

		for (i = 0; i < 500; ++i)
		{
			int readRes = read(a, board, BUFFER_SIZE);	
			ASSERT(readRes == BUFFER_SIZE);
		}		

		nextMove = RIGHT;
		writeval = write(a, &nextMove, 1);	
		ASSERT_INT_EQUALS(writeval,1);

		for (i = 0; i < 500; ++i)
		{
			int readRes = read(a, board, BUFFER_SIZE);	
			ASSERT(readRes == BUFFER_SIZE);
		}	

		nextMove = RIGHT;
		writeval = write(a, &nextMove, 1);	
		ASSERT(writeval == 1);

		for (i = 0; i < 500; ++i)
		{
			int readRes = read(a, board, BUFFER_SIZE);	
			ASSERT(readRes == BUFFER_SIZE);
		}	

		nextMove = RIGHT;
		writeval = write(a, &nextMove, 1);	
		ASSERT(writeval == 1);

		for (i = 0; i < 500; ++i)
		{
			int readRes = read(a, board, BUFFER_SIZE);	
			ASSERT(readRes == BUFFER_SIZE);
		}	

		doLongTask();
		close(a);
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			char nextMove = UP;
			char board[BUFFER_SIZE+1];
			board[BUFFER_SIZE]='\0'; // <---- might be redundant but i cant leave it like that! it's a disturbia

			doLongTask();
			//player 2 (black)
			b=open(moduleName, O_RDWR); 
			ASSERT(b>=0);

			int i;
			for (i = 0; i < 500; ++i)
			{
				int readRes = read(b, board, BUFFER_SIZE);	
				ASSERT(readRes == BUFFER_SIZE);
			}

			int writeval = write(b, &nextMove, 1);	
			ASSERT(writeval == 1);

			for (i = 0; i < 500; ++i)
			{
				int readRes = read(b, board, BUFFER_SIZE);	
				ASSERT(readRes == BUFFER_SIZE);
			}		

			nextMove = RIGHT;
			writeval = write(b, &nextMove, 1);	
			ASSERT(writeval == 1);

			for (i = 0; i < 500; ++i)
			{
				int readRes = read(b, board, BUFFER_SIZE);	
				ASSERT(readRes == BUFFER_SIZE);
			}	

			nextMove = RIGHT;
			writeval = write(b, &nextMove, 1);	
			ASSERT(writeval == 1);

			for (i = 0; i < 500; ++i)
			{
				int readRes = read(b, board, BUFFER_SIZE);	
				ASSERT(readRes == BUFFER_SIZE);
			}	

			nextMove = RIGHT;
			writeval = write(b, &nextMove, 1);	
			ASSERT(writeval == 1);

			for (i = 0; i < 500; ++i)
			{
				int readRes = read(b, board, BUFFER_SIZE);	
				ASSERT(readRes == BUFFER_SIZE);
			}	

			doLongTask();
			close(b);
			_exit(0);
		} else {
			wait(&status);
		}
		wait(&status);
	}	
    return true;	
}


//todo - get string instead of char!
void PlayFullGame()
{
	isGameBoy = 1;
	char nextMoveAux;
	int lastPlayer = 0;
	
	numOfPlayers = mmap(NULL, sizeof (*numOfPlayers), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*numOfPlayers = 0;

	gameHasEnded = mmap(NULL, sizeof (*gameHasEnded), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*gameHasEnded = -1; // game will be over if gameHasEnded is != -1
	
	nextMove = mmap(NULL, sizeof(*nextMove), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	player = mmap(NULL, sizeof (*player), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*player = 100;

	Currentplayer = mmap(NULL, sizeof (*Currentplayer), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*Currentplayer = 0;

	a = mmap(NULL, sizeof (*a), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	b = mmap(NULL, sizeof (*b), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	char* moduleName = "/dev/snake17";
	int status;
	int pid1 = fork();
	if(pid1 == 0) {
		char board[BUFFER_SIZE+1];
		board[BUFFER_SIZE] = '\0';
		//player 1 (white)
		*a=open(moduleName, O_RDWR);
		*numOfPlayers = *numOfPlayers +1;
		while(*gameHasEnded == -1)
		{
			if(*player == 1)
			{
				*player = 0;
				*Currentplayer = 1;
				read(*a, board, BUFFER_SIZE); //board before
				printButtonPress(board,nextMove);

				int res = write(*a,nextMove, 1);
				if(res < 0 ) {*gameHasEnded =1;}

				read(*a, board, BUFFER_SIZE);
				printBoard(board);
				*player = 100;
			}
		}
		_exit(0);
	} else {
		int pid2 = fork();
		if(pid2 == 0) {
			char board[BUFFER_SIZE];
			doLongTask();
			//player 2 (black)
			*b=open(moduleName, O_RDWR);
			*Currentplayer = -1;
			read(*b, board, BUFFER_SIZE);
			printBoard(board);
			*numOfPlayers = *numOfPlayers +1;
			while(*gameHasEnded == -1)
			{
				if(*player == -1)
				{
					*player = 0;
					*Currentplayer = -1;
					read(*b, board, BUFFER_SIZE);
					printButtonPress(board,nextMove);
					int res = write(*b,nextMove, 1);
					if(res < 0 ) {*gameHasEnded =1;}

					read(*b, board, BUFFER_SIZE);
					printBoard(board);
					*player = 100;
					*gameHasEnded = ioctl(*b, SNAKE_GET_WINNER);
				}
			}
			_exit(0);
		} else {
			printLogo();
			doLongTask();
			lastPlayer = -1;
			//manager
			while(*gameHasEnded == -1)
			{
				if(*player == 100 && *numOfPlayers==2)
				{
					char nm;
					scanf(" %c", &nm);
					if(nm != 'a' && nm != 'A' 
						&& nm != 's' && nm != 'S'
						&& nm != 'w' && nm != 'W'
						&& nm != 'd' && nm != 'D'

						&& nm != '2' && nm != '4'
						&& nm != '6' && nm != '8')
					{
						continue;
					}

					if (nm == 'w' || nm == 'W' || nm == '8')
					{     
						nextMoveAux = UP;
					}
					else if (nm == 's' || nm == 'S' || nm == '2')
					{
				    	nextMoveAux = DOWN;
					}
					else if (nm == 'a' || nm == 'A' || nm == '4')
					{
				    	nextMoveAux = LEFT;
					}
					else if (nm == 'd' || nm == 'D' || nm == '6')
					{
				    	nextMoveAux = RIGHT;
					}

					*nextMove = nextMoveAux; 
					*player = lastPlayer * (-1);
					lastPlayer = *player;
				}
			}
			printGameOver();
		    close(*a);
    		close(*b);	
			wait(&status);
		}
		wait(&status);
	}
	munmap(numOfPlayers, sizeof (*numOfPlayers));
	munmap(gameHasEnded, sizeof (*gameHasEnded));
	munmap(nextMove, sizeof (*nextMove));
	munmap(player, sizeof (*player));
	munmap(Currentplayer, sizeof (*Currentplayer));
	munmap(a, sizeof (*a));
	munmap(b, sizeof (*b));
}


/*========================[TESTS]========================*/


int main(){
	setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"); 

	//printWelcome();
	printf("Enter A or B\n");
	char answer;
	while (scanf(" %c", &answer) == 1 && (answer != 'A' && answer != 'a' && answer != 'b' && answer != 'B')){
		printf("\033[2A");
		printf("Wrong input - Press A to play, Press B to run test\n");
	}
	printf("\033[2A");
	printf("                                                   \n                                                   \n");
	printf("\033[2A");

	if(answer == 'B' || answer == 'b')
	{
		cleanScreen();
		char* running = "_n________________________\n|_|______________________|_|\n|  ,--------------------.  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  |***--------------***|  |\n|  |**-[RUNNING TEST]-**|  |\n|  |***--------------***|  |\n|  |********************|  |\n|  |********************|  |\n|  |********************|  |\n|  `--------------------'  |\n|      _  GAME BOY         |\n|    _| |_         ,-.     |\n|   |_ O _|   ,-. \"._,\"    |\n|     |_|    \"._,\"   A     |\n|       _  _    B          |\n|      // //               |\n|     // //     \\\\\\\\\\\\     |\n|     `  `       \\\\\\\\\\\\    ,\n|___________...__________,\"\n";
		printf("%s\n", running);
		printf("\n\n");
        //RUN_TEST(CreateNewGameTest); //OK
        //RUN_TEST(CreateGameSameModuleAfterCancelTest); //OK
        //RUN_TEST(CreateGameWithThreePlayersTest); //OK
        //RUN_TEST(PrintBoardTest); //OK
        //RUN_TEST(MyFirstMoveTest); //OK
        //RUN_TEST(MakeMoveCrashWallTest); //OK
        //RUN_TEST(MakeMoveIlligalCharacterTest); //OK
        //RUN_TEST(MakeMultipleMovesTest); //OK
        //RUN_TEST(MakeMoveCrashSnakeTest); //OK
        //RUN_TEST(MakeMultipleMovesWithIlligalMoveTest);  //<----assert, line 933
        //RUN_TEST(SnakeGetColorTest); //OK
        //RUN_TEST(GetWinnerWhiteWinTest); //OK
        //RUN_TEST(GetWinnerBlackWinTest); //OK
        //RUN_TEST(GetWinnerGameInProgressTest); // OK
        //RUN_TEST(TreeGamesAtTheSameTime); //OK
        //RUN_TEST(TwoWhitesAgainstOneBlackTest); //OK
        RUN_TEST(ThreeAgainstThreeTest); //<<----------------------------------------
        RUN_TEST(AgressiveReadWriteTest); //<<----------------------------------------
        
	}
	else if (answer == 'a' || answer == 'A')
	{
		PlayFullGame();
	}
	
    return 0;
}
