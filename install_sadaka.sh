#!/bin/bash

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


num_games=50
for (( i = 0; i <= num_games; i++ )); do
	rm /dev/snake${i}
done

rmmod snake
runCommand make 
insmod ./snake.o max_games=${num_games}

for (( i = 0; i <= num_games; i++ )); do
	mknod /dev/snake${i} c 254 ${i}
done
