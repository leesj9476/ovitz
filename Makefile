CC=g++
CFLAGS=-g -Wall -Wpedantic -Wextra --std=c++11 -lpthread
SRCS_DIR=src
OBJS_DIR=obj

OBJS=$(addprefix $(OBJS_DIR)/, main.o image.o capture.o ssd1306_i2c.o util.o)
DEST=ovitz

OPENCV=`pkg-config opencv --cflags --libs`
OPENCV_LIBS=$(OPENCV)

PICAMERA_LIBS=-lraspicam -lraspicam_cv

WIRINGPI_LIBS=-lwiringPi

CONF_DIR=conf
SCRIPTS=init.sh ovitz.sh
SCRIPTS_DIR=scripts
SCRIPTS_HOME_DIR=$(HOME)/rc.scripts

LOCK_DIR=lock

ALL_DIRS=$(OBJS_DIR) $(SCRIPTS_HOME_DIR) $(LOCK_DIR) 

all: init $(DEST)

$(DEST): $(OBJS)
	$(CC) $(CFLAGS) $(OPENCV_LIBS) $(PICAMERA_LIBS) $(WIRINGPI_LIBS) $(OBJS) -o $(DEST)

$(OBJS_DIR)/main.o:
	$(CC) $(CFLAGS) -c $(SRCS_DIR)/main.cpp -o $(OBJS_DIR)/main.o

$(OBJS_DIR)/image.o:
	$(CC) $(CFLAGS) $(OPENCV_LIBS) -c $(SRCS_DIR)/image.cpp -o $(OBJS_DIR)/image.o

$(OBJS_DIR)/capture.o:
	$(CC) $(CFLAGS) $(OPENCV_LIBS) $(PICAMERA_LIBS) $(WIRINGPI_LIBS) -c $(SRCS_DIR)/capture.cpp -o $(OBJS_DIR)/capture.o

$(OBJS_DIR)/ssd1306_i2c.o:
	$(CC) $(CFLAGS) $(WIRINGPI_LIBS) -c $(SRCS_DIR)/ssd1306_i2c.c -o $(OBJS_DIR)/ssd1306_i2c.o

$(OBJS_DIR)/util.o:
	$(CC) $(CFLAGS) $(PICAMERA_LIBS) -c $(SRCS_DIR)/util.cpp -o $(OBJS_DIR)/util.o

init:
	make clean
	mkdir $(ALL_DIRS)
	touch $(LOCK_DIR)/window_avail
	sudo chmod 755 $(SCRIPTS_DIR)/init.sh $(SCRIPTS_DIR)/ovitz.sh
	cp $(SCRIPTS_DIR)/init.sh $(SCRIPTS_DIR)/ovitz.sh $(SCRIPTS_HOME_DIR)

clean:
	rm -f $(DEST)
	sudo rm -rf $(ALL_DIRS)
