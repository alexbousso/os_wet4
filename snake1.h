#ifndef _SNAKE1_H_
#define _SNAKE1_H_

/*-------------------------------------------------------------------------
Include files:
--------------------------------------------------------------------------*/
#include <linux/ctype.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h> 
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/system.h>
#include <asm/uaccess.h>

#include <linux/sched.h>
#include <asm/semaphore.h>
#include <linux/random.h>

/*=========================================================================
Constants and definitions:
==========================================================================*/
#define N (4) /* the size of the board */
#define M (3)  /* the initial size of the snake */
#define K (5)  /* the number of turns a snake can survive without eating */

typedef char Player;
/* PAY ATTENTION! i will use the fact that white is positive one and black is negative
one to describe the segments of the snake. for example, if the white snake is 2 segments
long and the black snake is 3 segments long
white snake is  1   2
black snake is -1  -2  -3 */
#define WHITE ( 1) /* id to the white player */
#define BLACK (-1) /* id to the black player */
#define EMPTY ( 0) /* to describe an empty point or no player*/

#define GAME_ON ( 0)
#define TIE     ( 5)
/* to describe a point with food. having the value this big guarantees that there will be no
overlapping between the snake segments' numbers and the food id */
#define FOOD  (N*N)

typedef char bool_t;
#define FALSE (0)
#define TRUE  (1)

typedef int Direction;
#define DOWN  (2)
#define LEFT  (4)
#define RIGHT (6)
#define UP    (8)

/* a point in 2d space */
typedef struct{
	int x, y;
} Point;

typedef int Matrix[N][N];

typedef int ErrorCode;
#define ERR_OK      			((ErrorCode) 0)
#define ERR_BOARD_FULL			((ErrorCode)-1)
#define ERR_SNAKE_IS_TOO_HUNGRY ((ErrorCode)-2)

struct game {
	struct semaphore sem_begin_game;
	struct semaphore sem_game_data;
	struct semaphore sem_white_players;
	struct semaphore sem_black_players;
	int num_of_players;
	Player curr_player;
	Matrix board;
	Player winner;
	int white_counter;
	int black_counter;
};


int my_open( struct inode *inode, struct file *filp );
int my_release( struct inode *inode, struct file *filp );
ssize_t my_read( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
loff_t my_llseek(struct file *filp, loff_t a, int b);
int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

bool_t Init(Matrix*); /* initialize the board. return false if the board is illegal (should not occur, affected by N, M parameters) */
bool_t Update(struct game* g, Direction move);/* handle all updating to this player. returns whether to continue or not. */
void Print(Matrix*, char*, size_t);/* prints the state of the board */
Point GetInputLoc(Matrix*, Player, Direction);/* calculates the location that the player wants to go to */
bool_t CheckTarget(Matrix*, Player, Point);/* checks if the player can move to the specified location */
Point GetSegment(Matrix*, int);/* gets the location of a segment which is numbered by the value */
bool_t IsAvailable(Matrix*, Point);/* returns if the point wanted is in bounds and not occupied by any snake */
ErrorCode CheckFoodAndMove(struct game*, Point);/* handle food and advance the snake accordingly */
ErrorCode RandFoodLocation(Matrix*);/* randomize a location for food. return ERR_BOARD_FULL if the board is full */
void AdvancePlayer(Matrix*, Player, Point);/* advance the snake */
void IncSizePlayer(Matrix*, Player, Point);/* advance the snake and increase it's size */
bool_t IsMatrixFull(Matrix*);/* check if the matrix is full */
int GetSize(Matrix*, Player);/* gets the size of the snake */

#endif /* _SNAKE1_H_ */
