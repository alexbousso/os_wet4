#ifndef _SNAKE_H_
#define _SNAKE_H_

#include <linux/ioctl.h>

#define SNAKE_IOC_MAGIC 'D'
#define SNAKE_GET_WINNER  _IOR(SNAKE_IOC_MAGIC, 0, int)
#define SNAKE_GET_COLOR   _IOR(SNAKE_IOC_MAGIC, 1, int)

#endif /* _SNAKE_H_ */
