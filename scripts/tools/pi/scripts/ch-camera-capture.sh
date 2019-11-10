#!/bin/bash

_signal() {
    echo "Caught SIGTERM signal!" 
    kill -TERM "$child" 2>/dev/null
}

trap _signal SIGTERM SIGINT

raspivid -a 4 -a "Garage 1 | %A %Y-%m-%d %X %Z" -w 848 -h 480 -fps 24 -vf -t 0 -b 400000 --rotation 180 -o /security/fifo.h264 &

child=$! 
wait "$child"

