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

#define NULL_ARG		-EFAULT
#define COPY_USER_FAIL	-EFAULT
#define MEMORY_FAIL		-ENOMEM
#define MAX_PLAYERS		-EPERM
#define ILLEGAL_MINOR	-ENODEV
#define GENERAL_ERROR	-EINVAL
#define PLAYER_QUIT		-10
#define GAME_OVER		13 	//<-----


struct file_operations my_fops = {
	.open=		my_open,
	.release=	my_release,
	.read=		my_read,
	.write=		my_write,
	.llseek=	my_llseek,
	.ioctl=		my_ioctl,
};

int max_games = 0;
MODULE_PARM( max_games, "i" );

/* globals */
int my_major = 0; 			/* will hold the major # of my device driver */
struct game* games = NULL;

int init_module( void ) {
	//dbg_print(7, "In init_module()\n");
	SET_MODULE_OWNER(&my_fops);
	my_major = register_chrdev( my_major, MY_MODULE, &my_fops );
	if( my_major < 0 ) {
		printk(KERN_WARNING "ERROR: can't get dynamic major\n" );
		return my_major;
	}
	
	//dbg_print(1, "my_major = %d\n", my_major);
	
	games = kmalloc(sizeof(struct game) * max_games, GFP_KERNEL);
	if (!games) {
		//dbg_print(10, "ERROR: Memory error!\n");
		return MEMORY_FAIL;
	}
	
	for(int i=0; i<max_games; i++){
		init_MUTEX_LOCKED(&(games[i].sem_begin_game));
		init_MUTEX(&(games[i].sem_game_data));
		init_MUTEX(&(games[i].sem_white_players));
		init_MUTEX(&(games[i].sem_black_players));
		games[i].num_of_players = 0;
		games[i].curr_player = 0;
		games[i].white_counter = K;
		games[i].black_counter = K;
		if (!Init(&(games[i].board))){
			return GENERAL_ERROR; // ERRNO????<--------------------------------
		}
		games[i].winner = 0;
	}
	
	return 0;
}


void cleanup_module( void ) { //what about the case that cleanup_module is called and not all the games are over?
	//release semaphore
	unregister_chrdev(my_major, MY_MODULE);
	
	kfree(games);

	return;
}


int my_open( struct inode *inode, struct file *filp ) {
	if(!inode || !filp) return NULL_ARG; //ERRNO???
	int minor = MINOR(inode->i_rdev);
	if(minor < 0 || minor > max_games-1){
		return ILLEGAL_MINOR;
	}
	down(&games[minor].sem_game_data); //lock
	if(games[minor].num_of_players >= 2 || games[minor].winner){
		up(&games[minor].sem_game_data); //unlock
		return MAX_PLAYERS;
	}
	games[minor].num_of_players++;
	filp->private_data = (int*) kmalloc (sizeof(int)*2, GFP_KERNEL); // [0]=minor, [1]=colour;
	if(filp->private_data) ((int *)filp->private_data)[0] = minor;
	else { //malloc failed
		up(&games[minor].sem_game_data); //unlock
		return MEMORY_FAIL;
	}
	if(games[minor].num_of_players == 1){ //first player 
		((int *)filp->private_data)[1] = WHITE;
		up(&games[minor].sem_game_data); //unlock
		down(&(games[minor].sem_begin_game)); //lock
	}
	else { //second player
		((int *)filp->private_data)[1] = BLACK;
		games[minor].curr_player = WHITE;
		up(&games[minor].sem_game_data); //unlock
		up(&(games[minor].sem_begin_game)); //unlock
	}
	return 0;
}


int my_release( struct inode *inode, struct file *filp ) {
	if(!inode || !filp) return NULL_ARG; //ERRNO???
	int minor = MINOR(inode->i_rdev);
	down(&games[minor].sem_game_data); //lock
	games[minor].num_of_players--;
	
	if(!games[minor].winner){ //game still on, and one of the players is leaving
		games[minor].winner = -(((int *)filp->private_data)[1]);
	}
	
	if(filp->private_data) kfree(filp->private_data);
	filp->private_data = 0;
	up(&games[minor].sem_game_data); //unlock
	return 0;
}


ssize_t my_read( struct file *filp, char *buf, size_t count, loff_t *f_pos ) {
	char* tmp_buf;
	int minor;
	if(!filp || (!buf && count))	return NULL_ARG;
	if(!count) return 0;
	minor = ((int *)filp->private_data)[0];
	down(&games[minor].sem_game_data); //lock
	if(games[minor].num_of_players != 2){
		up(&games[minor].sem_game_data); //unlock
		return PLAYER_QUIT;
	}
	tmp_buf = (char*) kmalloc(count, GFP_KERNEL);
	if(!tmp_buf){
		up(&games[minor].sem_game_data); //unlock
		return MEMORY_FAIL;
	}
	Print(&(games[minor].board), tmp_buf, count);
	int not_copied = copy_to_user(buf, tmp_buf, (unsigned long)count);
	kfree(tmp_buf);
	up(&games[minor].sem_game_data); //unlock
	return (not_copied == count ? COPY_USER_FAIL : count-not_copied);
}


ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	int minor = ((int *)filp->private_data)[0];
	int player =  ((int *)filp->private_data)[1];
	char* kernel_buf;
	int k_buf_pos = 0, orig_size = count;
	
	if(!filp || (!buf && count)) 	return NULL_ARG;
	if(count == 0) 					return 0;
	
	down(&games[minor].sem_game_data); //lock
	if(games[minor].num_of_players != 2){
		up(&games[minor].sem_game_data); //unlock
		return PLAYER_QUIT;
	}
	if(games[minor].winner != EMPTY){
		up(&games[minor].sem_game_data); //unlock
		return GAME_OVER;
	}

	kernel_buf = (char*) kmalloc (count, GFP_KERNEL);
	if(!kernel_buf){
		up(&games[minor].sem_game_data); //unlock
		return MEMORY_FAIL;
	}
	copy_from_user(kernel_buf,buf,(unsigned long)count);
	up(&games[minor].sem_game_data); //unlock
	
	if(player == WHITE) down(&games[minor].sem_white_players); //lock
	else down(&games[minor].sem_black_players); //lock
	
	while(count){ //there are still moves in buff
		down(&games[minor].sem_game_data); //lock
		Direction move;
		while((games[minor].curr_player != player) && (games[minor].winner == EMPTY)){
			up(&games[minor].sem_game_data); //unlock
			schedule();
			down(&games[minor].sem_game_data); //lock
		}
		//my turn! play:
		move = kernel_buf[k_buf_pos++] - '0';
		if (games[minor].winner || (move != UP && move != DOWN && move != LEFT && move != RIGHT)){
			kfree(kernel_buf);
			if(player == WHITE) up(&games[minor].sem_white_players); //unlock
			else up(&games[minor].sem_black_players); //unlock
			if(games[minor].winner == EMPTY){
				games[minor].winner = -player;
			}
			else {
				up(&games[minor].sem_game_data); //unlock
				return GAME_OVER;
			}
			up(&games[minor].sem_game_data); //unlock
			return (k_buf_pos > 1 ? k_buf_pos-1 : -1);
		}
		else{
			if(!Update(&(games[minor]), move)){
				kfree(kernel_buf);
				up(&games[minor].sem_game_data); //unlock
				if(player == WHITE) up(&games[minor].sem_white_players); //unlock
				else up(&games[minor].sem_black_players); //unlock
				return k_buf_pos;
			}
		}
		games[minor].curr_player = -player;
		up(&games[minor].sem_game_data); //unlock
		count--;
		buf++;
	}
	kfree(kernel_buf);
	if(player == WHITE) up(&games[minor].sem_white_players); //unlock
	else up(&games[minor].sem_black_players); //unlock
	
	return orig_size;
}

loff_t my_llseek(struct file *filp, loff_t a, int b) {
	return -ENOSYS;
}

int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
	int minor = ((int *)filp->private_data)[0];
	int player =  ((int *)filp->private_data)[1];
	int winner;
	switch(cmd) {
		case SNAKE_GET_WINNER:
			down(&games[minor].sem_game_data); //lock
			winner = games[minor].winner;
			up(&games[minor].sem_game_data); //unlock
			if(winner == WHITE) return 4;
			else if(winner == BLACK) return 2;
			else if(winner == TIE )  return TIE;
			else return -1;
			break;
		case SNAKE_GET_COLOR:
			if(player == WHITE) return 4;
			else if(player == BLACK) return 2;
			else return -1;
			break;
		default: return -ENOTTY;
	}
	return 0;
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// hw3q1.c :
bool_t Init(Matrix *matrix)
{
	for(int i=0; i<N; i++){
		for(int j=0; j<N; j++){
			(*matrix)[i][j] = EMPTY;
		}
	}
	
	//dbg_print(7, "In Init()\n");
	int i;
	/* initialize the snakes location */
	for (i = 0; i < M; ++i)
	{
		(*matrix)[0][i] =   WHITE * (i + 1);
		(*matrix)[N - 1][i] = BLACK * (i + 1);
	}
	
	//dbg_print(1, "Before RandFoodLocation()\n");
	/* initialize the food location */
	if (RandFoodLocation(matrix) != ERR_OK)
		return FALSE;
	//dbg_print(1, "After RandFoodLocation()\n");
	return TRUE;
}

bool_t Update(struct game* g, Direction move)//(Matrix *matrix, Player player, Player* winner, Direction move)
{
	ErrorCode e;
	Point p = GetInputLoc(&(g->board), g->curr_player, move);

	if (!CheckTarget(&(g->board), g->curr_player, p)){// printf("% d lost.", player);
		g->winner = -(g->curr_player);
		return FALSE;
	}

	e = CheckFoodAndMove(g,p);//(&(g->board), g->curr_player, p);
	if (e == ERR_BOARD_FULL){ //printf("the board is full, tie");
		g->winner = TIE;
		return FALSE;
	}
	if (e == ERR_SNAKE_IS_TOO_HUNGRY){ //printf("% d lost. the snake is too hungry", player);
		g->winner = -(g->curr_player);
		return FALSE;
	}
	// only option is that e == ERR_OK
	if (IsMatrixFull(&(g->board))){ //printf("the board is full, tie");
		g->winner = TIE;
		return FALSE;
	}

	return TRUE;
}

Point GetInputLoc(Matrix *matrix, Player player, Direction move)
{
	Point p;

	p = GetSegment(matrix, player);

	switch (move)
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

bool_t CheckTarget(Matrix *matrix, Player player, Point p)
{
	/* is empty or is the tail of the snake (so it will move the next
	to make place) */
	return IsAvailable(matrix, p) || ((*matrix)[p.y][p.x] == player * GetSize(matrix, player));
}

bool_t IsAvailable(Matrix *matrix, Point p)
{
	return
		/* is out of bounds */
		!(p.x < 0 || p.x >(N - 1) ||
		p.y < 0 || p.y >(N - 1) ||
		/* is empty */
		((*matrix)[p.y][p.x] != EMPTY && (*matrix)[p.y][p.x] != FOOD));
}

ErrorCode CheckFoodAndMove(struct game* g, Point p)//(Matrix *matrix, Player player, Point p)
{
	Matrix *matrix = &(g->board);
	//static int white_counter = K;
	//static int black_counter = K;
	/* if the player did come to the place where there is food */
	if ((*matrix)[p.y][p.x] == FOOD)
	{
		if (g->curr_player == BLACK) g->black_counter = K;
		if (g->curr_player == WHITE) g->white_counter = K;

		IncSizePlayer(matrix, g->curr_player, p);

		if (RandFoodLocation(matrix) != ERR_OK)
			return ERR_BOARD_FULL;
	}
	else /* check hunger */
	{
		if (g->curr_player == BLACK && --(g->black_counter) == 0)
			return ERR_SNAKE_IS_TOO_HUNGRY;
		if (g->curr_player == WHITE && --(g->white_counter) == 0)
			return ERR_SNAKE_IS_TOO_HUNGRY;

		AdvancePlayer(matrix, g->curr_player, p);
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
		get_random_bytes(&(p.x),sizeof(p.x));
		if(p.x < 0) p.x *= -1;
		p.x = p.x % N;
		//dbg_print(1, "&p.x mod(n) after random = %d \n", p.x);
		get_random_bytes(&(p.y),sizeof(p.y));
		if(p.y < 0) p.y *= -1;
		p.y = p.y % N;
		//dbg_print(1, "&p.y mod(n) after random = %d \n", p.x);
	} while (!IsAvailable(matrix, p) || IsMatrixFull(matrix));

	if (IsMatrixFull(matrix))
		return ERR_BOARD_FULL;

	(*matrix)[p.y][p.x] = FOOD;
	return ERR_OK;
}

bool_t IsMatrixFull(Matrix *matrix)
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

void Print(Matrix *matrix, char* buff, size_t count)
{
	Point p;
	char tmp_buff[3*N*N + 10*N + 10]; //3*N*N + 10*N + 8 = (3N+4)(N+2)= (#cols)*(#rows)
	int tmp_buff_size = 3*N*N + 10*N + 10;
	int i = 0, tmp_buff_curr = 0;
	
	for(i = 0; i < tmp_buff_size; i++)
		tmp_buff[i] = 0;
	for(i = 0; i < count; i++)
		buff[i] = 0;
	
	for (i = 0; i < N + 1; ++i){
		strncpy(tmp_buff+tmp_buff_curr,"---",3);
		tmp_buff_curr+=3;
	}
	strncpy(tmp_buff+tmp_buff_curr,"\n",1);
	tmp_buff_curr+=1;
	
	for (p.y = 0; p.y < N; ++p.y)
	{
		strncpy(tmp_buff+tmp_buff_curr,"|",1);
		tmp_buff_curr+=1;
		for (p.x = 0; p.x < N; ++p.x)
		{
			switch ((*matrix)[p.y][p.x])
			{
			case FOOD:
				strncpy(tmp_buff+tmp_buff_curr,"  *",3);
				tmp_buff_curr+=3;
				break;
			case EMPTY:
				strncpy(tmp_buff+tmp_buff_curr,"  .",3);
				tmp_buff_curr+=3;
				break;
			default: //printf("% 3d", (*matrix)[p.y][p.x]);
				tmp_buff[tmp_buff_curr++]=' ';
				int num = (*matrix)[p.y][p.x];
				if(num < 0){
					tmp_buff[tmp_buff_curr++] = '-';
					num *= -1;
				}
				else tmp_buff[tmp_buff_curr++] = ' ';
				tmp_buff[tmp_buff_curr++] = num + '0';
			}
		}
		strncpy(tmp_buff+tmp_buff_curr," |\n",3);
		tmp_buff_curr+=3;
	}
	
	for (i = 0; i < N + 1; ++i){
		strncpy(tmp_buff+tmp_buff_curr,"---",3);
		tmp_buff_curr+=3;
	}
	strncpy(tmp_buff+tmp_buff_curr,"\n",1);
	tmp_buff_curr+=1;
	
	strncpy(buff, tmp_buff, count);
}

