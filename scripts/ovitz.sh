#! /bin/sh

window_file="./lock/window_avail"

cd /home/pi/workspace/ovitz
./ovitz

if [ ! -e "$window_file" ]; then
	touch "$window_file"
fi

cd /home/pi
