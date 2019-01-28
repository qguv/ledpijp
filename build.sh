#!/bin/bash

DEBUG=""
if [ "$1" == "-d" ]; then
	DEBUG="-D DEBUG"
fi

gcc ${DEBUG} -lm -lrt -Wall -Wextra -Wpedantic -Werror *.c -o ledpijp
