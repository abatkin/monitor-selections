#!/bin/sh

gcc -Wall -pedantic -o monitor -lX11 monitor.c -ggdb -std=c99 && ./monitor

