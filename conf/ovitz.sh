# /bin/sh

window_file="./lock/window_avail"

cd /home/pi/workspace/ovitz
./start

if [ ! -e "$window_file" ]; then
	touch "$window_file"
fi

cd /home/pi
