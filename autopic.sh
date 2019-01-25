#!/bin/sh

current_time=$(date "+%Y-%m-%d_%H%M%S")
echo "Current time $current_time\n"
filename900="900IR$current_time.jpg"
filename800="800NIR$current_time.jpg"

#cameraparms='-ex off -t 1000 -ISO 400 -awb sun -ss '

gpio mode 4 out
gpio mode 5 out

gpio write 4 0
gpio write 5 0
raspistill $1 -o $filename900

gpio write 4 1
gpio write 5 1

raspistill $1 -o $filename800


#extractRAW $filename $2
