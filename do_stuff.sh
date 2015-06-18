#!/bin/bash

NUM_OF_GAMES=50

function runCommand {
    command=$1
    if [[ "${command}" == "" ]]; then
        echo "runCommand: Got an empty command!"
        return
    fi

    echo "runCommand: Running ${command}"
    ${command}
    retval=$?
    if [[ ${retval} -ne 0 ]]; then
        echo "********************************************************************************"
        echo "Error:"
        echo "runCommand: Command \"${command}\" failed and returned: ${retval}"
        echo "Aborting compilation"
        echo "********************************************************************************"
        # Exit script
        exit 1
    fi
}

if [[ $# -ne 1 ]]; then
	echo "Usage: do_stuff.sh [install | uninstall]"
	exit 1
fi

if [[ $1 = "install" ]]; then
	runCommand "make"
	insmod ./snake.o max_games=$NUM_OF_GAMES

	mod=$(cat /proc/devices | grep snake | cut -d ' ' -f1)
	for (( i = 0; i < NUM_OF_GAMES; i++ )); do
		mknod /dev/snake$i c $mod $i
	done

	gcc -std=c99 -g -DVERBOSE -Wall tests.c
	
	exit 0
fi

if [[ $1 = "uninstall" ]]; then
	rm -f ./a.o
	rm -f ./a.out
	rm -f /dev/snake*
	rmmod snake
	
	exit 0
fi

