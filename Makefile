KERNELDIR = /usr/src/linux-2.4.18-14custom
include $(KERNELDIR)/.config
CFLAGS = -D__KERNEL__ -DMODULE -I$(KERNELDIR)/include -O -Wall -std=gnu99
all : snake.o
