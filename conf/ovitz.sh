# /bin/sh

file="./lock/window_avail"

cd /home/pi/workspace/ovitz
./start

if [ ! -e "$file" ]; then
	touch "$file"
fi

cd /home/pi
