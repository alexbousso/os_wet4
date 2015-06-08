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
#include <asm/semaphore.h>

#include "snake.h"
#include "snake1.h"

#ifdef DEBUG_ON

#ifndef PRIORITY
#define PRIORITY 0
#endif /* PRIORITY */

#define dbg_print(priority, format, args...) 	\
	if ((priority) >= PRIORITY)				\
		printk((format), ## args);			\

#else /* not defined DEBUG_ON */

#define dbg_print(priority, format, args...)

#endif /* DEBUG_ON */

#define MY_MODULE "snake"

MODULE_LICENSE( "GPL" );

struct file_operations my_fops = {
	.open=		my_open,
	.release=	my_release,
	.read=		my_read,
	.write=		my_write,
	.llseek=		NULL,
	.ioctl=		my_ioctl,
	.owner=		THIS_MODULE,
};

struct game {
	struct semaphore sem_begin_game;
	struct semaphore sem_game_data;
	int num_of_players;
	Player curr_player;
	Matrix board;
	Player winner;
};

int max_games = 0;
MODULE_PARM( max_games, "i" );

/* globals */
int my_major = 0; 			/* will hold the major # of my device driver */
struct game* games = NULL;

int init_module( void ) {
	my_major = register_chrdev( my_major, MY_MODULE, &my_fops );
	if( my_major < 0 ) {
		printk(10, KERN_WARNING "ERROR: can't get dynamic major\n" ); //changed to printk <-------
		return my_major;
	}
	
	games = malloc(sizeof(struct game *) * max_games);
	if (!games) {
		printk(10, "ERROR: Memory error!\n"); //changed to printk <-------
		return -1;
	}
	
	for(int i=0; i<max_games; i++){
		init_MUTEX_LOCKED(&(games[i].sem_begin_game));
		init_MUTEX(&(games[i].sem_game_data));
		games[i].num_of_players = 0;
		games[i].curr_player = 0;
		if (!Init(&(games[i].board))){
			return -1; // return value when init failes??? <--------------------------------
		}
		games[i].winner = 0;
	}
	
	return 0;
}


void cleanup_module( void ) {
	
	unregister_chrdev( my_major, MY_MODULE);
	
	free(games);

	return;
}


int my_open( struct inode *inode, struct file *filp ) {
	int minor = MINOR(inode->i_rdev);
	down(games[minor].sem_game_data); //lock
	if(minor < 0 || minor > max_games-1 || games[minor].num_of_players >= 2 || games[minor].winner){
		up(games[minor].sem_game_data); //unlock
		return -1; //which errno should we return?? <--------------------------
	}
	
	games[minor].num_of_players++;
	filp->private_data = (int*) malloc (sizeof(int)*2); // [0]=minor, [1]=colour;
	filp->private_data[0] = minor;
	if(games[minor].num_of_players == 1){ //first player 
		filp->private_data[1] = WHITE;
		up(games[minor].sem_game_data); //unlock
		down(&(games[minor].sem_begin_game)); //lock
	}
	else { //second player
		filp->private_data[1] = BLACK;
		up(games[minor].sem_game_data); //unlock
		up(&(games[minor].sem_begin_game)); //unlock
	}
	
	return 0;
}


int my_release( struct inode *inode, struct file *filp ) {
	int minor = MINOR(inode->i_rdev);
	down(games[minor].sem_game_data); //lock
	games[minor].num_of_players--;
	
	if(!games[minor].winner){ //game still on, and one of the players is leaving
		games[minor].winner = -(flip->private_data[1]);
	}
	
	free(filp->private_data);
	up(games[minor].sem_game_data); //unlock
	return 0;
}


ssize_t my_read( struct file *filp, char *buf, size_t count, loff_t *f_pos ) {
	//read the data
	//copy the data to user
    //return the ammount of read data
}


ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    //copy the data from user
	//write the data
    // return the ammount of written data
}



int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {

	switch( cmd ) {
		case MY_OP1:
            //handle op1;
			break;

		case MY_OP2:
			//handle op2;
			break;

		default: return -ENOTTY;
	}
	return 0;
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// hw3q1.c :
bool Init(Matrix *matrix)
{
	int i;
	/* initialize the snakes location */
	for (i = 0; i < M; ++i)
	{
		(*matrix)[0][i] =   WHITE * (i + 1);
		(*matrix)[N - 1][i] = BLACK * (i + 1);
	}
	/* initialize the food location */
	srand(time(0));
	if (RandFoodLocation(matrix) != ERR_OK)
		return FALSE;
	printf("instructions: white player is represented by positive numbers, \nblack player is represented by negative numbers\n");
	Print(matrix);

	return TRUE;
}

bool Update(Matrix *matrix, Player player)
{
	ErrorCode e;
	Point p = GetInputLoc(matrix, player);

	if (!CheckTarget(matrix, player, p))
	{
		printf("% d lost.", player);
		return FALSE;
	}
	e = CheckFoodAndMove(matrix, player, p);
	if (e == ERR_BOARD_FULL)
	{
		printf("the board is full, tie");
		return FALSE;
	}
	if (e == ERR_SNAKE_IS_TOO_HUNGRY)
	{
		printf("% d lost. the snake is too hungry", player);
		return FALSE;
	}
	// only option is that e == ERR_OK
	if (IsMatrixFull(matrix))
	{
		printf("the board is full, tie");
		return FALSE;
	}

	return TRUE;
}

Point GetInputLoc(Matrix *matrix, Player player)
{
	Direction dir;
	Point p;

	printf("% d, please enter your move(DOWN2, LEFT4, RIGHT6, UP8):\n", player);
	do
	{
		if (scanf("%d", &dir) < 0)
		{
			printf("an error occurred, the program will now exit.\n");
			exit(1);
		}
		if (dir != UP   && dir != DOWN && dir != LEFT && dir != RIGHT)
		{
			printf("invalid input, please try again\n");
		}
		else
		{
			break;
		}
	} while (TRUE);

	p = GetSegment(matrix, player);

	switch (dir)
	{
	case UP:    --p.y; break;
	case DOWN:  ++p.y; break;
	case LEFT:  --p.x; break;
	case RIGHT: ++p.x; break;
	}
	return p;
}

Point GetSegment(Matrix *matrix, int segment)
{
	/* just run through all the matrix */
	Point p;
	for (p.x = 0; p.x < N; ++p.x)
	{
		for (p.y = 0; p.y < N; ++p.y)
		{
			if ((*matrix)[p.y][p.x] == segment)
				return p;
		}
	}
	p.x = p.y = -1;
	return p;
}

int GetSize(Matrix *matrix, Player player)
{
	/* check one by one the size */
	Point p, next_p;
	int segment = 0;
	while (TRUE)
	{
		next_p = GetSegment(matrix, segment += player);
		if (next_p.x == -1)
			break;

		p = next_p;
	}

	return (*matrix)[p.y][p.x] * player;
}

bool CheckTarget(Matrix *matrix, Player player, Point p)
{
	/* is empty or is the tail of the snake (so it will move the next
	to make place) */
	return IsAvailable(matrix, p) || ((*matrix)[p.y][p.x] == player * GetSize(matrix, player));
}

bool IsAvailable(Matrix *matrix, Point p)
{
	return
		/* is out of bounds */
		!(p.x < 0 || p.x >(N - 1) ||
		p.y < 0 || p.y >(N - 1) ||
		/* is empty */
		((*matrix)[p.y][p.x] != EMPTY && (*matrix)[p.y][p.x] != FOOD));
}

ErrorCode CheckFoodAndMove(Matrix *matrix, Player player, Point p)
{
	static int white_counter = K;
	static int black_counter = K;
	/* if the player did come to the place where there is food */
	if ((*matrix)[p.y][p.x] == FOOD)
	{
		if (player == BLACK) black_counter = K;
		if (player == WHITE) white_counter = K;

		IncSizePlayer(matrix, player, p);

		if (RandFoodLocation(matrix) != ERR_OK)
			return ERR_BOARD_FULL;
	}
	else /* check hunger */
	{
		if (player == BLACK && --black_counter == 0)
			return ERR_SNAKE_IS_TOO_HUNGRY;
		if (player == WHITE && --white_counter == 0)
			return ERR_SNAKE_IS_TOO_HUNGRY;

		AdvancePlayer(matrix, player, p);
	}
	return ERR_OK;
}

void AdvancePlayer(Matrix *matrix, Player player, Point p)
{
	/* go from last to first so the identifier is always unique */
	Point p_tmp, p_tail = GetSegment(matrix, GetSize(matrix, player) * player);
	int segment = GetSize(matrix, player) * player;
	while (TRUE)
	{
		p_tmp = GetSegment(matrix, segment);
		(*matrix)[p_tmp.y][p_tmp.x] += player;
		segment -= player;
		if (segment == 0)
			break;
	}
	(*matrix)[p_tail.y][p_tail.x] = EMPTY;
	(*matrix)[p.y][p.x] = player;
}

void IncSizePlayer(Matrix *matrix, Player player, Point p)
{
	/* go from last to first so the identifier is always unique */
	Point p_tmp;
	int segment = GetSize(matrix, player)*player;
	while (TRUE)
	{
		p_tmp = GetSegment(matrix, segment);
		(*matrix)[p_tmp.y][p_tmp.x] += player;
		segment -= player;
		if (segment == 0)
			break;
	}
	(*matrix)[p.y][p.x] = player;
}

ErrorCode RandFoodLocation(Matrix *matrix)
{
	Point p;
	do
	{
		p.x = rand() % N;
		p.y = rand() % N;
	} while (!IsAvailable(matrix, p) || IsMatrixFull(matrix));

	if (IsMatrixFull(matrix))
		return ERR_BOARD_FULL;

	(*matrix)[p.y][p.x] = FOOD;
	return ERR_OK;
}

bool IsMatrixFull(Matrix *matrix)
{
	Point p;
	for (p.x = 0; p.x < N; ++p.x)
	{
		for (p.y = 0; p.y < N; ++p.y)
		{
			if ((*matrix)[p.y][p.x] == EMPTY || (*matrix)[p.y][p.x] == FOOD)
				return FALSE;
		}
	}
	return TRUE;
}

void Print(Matrix *matrix)
{
	int i;
	Point p;
	for (i = 0; i < N + 1; ++i)
		printf("---");
	printf("\n");
	for (p.y = 0; p.y < N; ++p.y)
	{
		printf("|");
		for (p.x = 0; p.x < N; ++p.x)
		{
			switch ((*matrix)[p.y][p.x])
			{
			case FOOD:  printf("  *"); break;
			case EMPTY: printf("  ."); break;
			default:    printf("% 3d", (*matrix)[p.y][p.x]);
			}
		}
		printf(" |\n");
	}
	for (i = 0; i < N + 1; ++i)
		printf("---");
	printf("\n");
}
