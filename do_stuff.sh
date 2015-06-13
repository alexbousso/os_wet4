#!/bin/bash

NUM_OF_GAMES=10

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

runCommand make
insmod ./snake.o max_games=$NUM_OF_GAMES

mod=$(cat /proc/devices | grep snake | cut -d ' ' -f1)

mknod /dev/game c $mod 3
cat /dev/game

