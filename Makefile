CC=g++
CFLAGS=-g -Wall -Wpedantic -Wextra --std=c++11 -lpthread
SRCS_DIR=src
OBJS_DIR=obj

OBJS=$(addprefix $(OBJS_DIR)/, main.o image.o capture.o oled.o util.o)
DEST=ovitz

WRAPPER_DEST=start
WRAPPER_OBJS=$(addprefix $(OBJS_DIR)/, start.o oled.o)

OPENCV=`pkg-config opencv --cflags --libs`
OPENCV_LIBS=$(OPENCV)

PICAMERA_LIBS=-lraspicam -lraspicam_cv

LED_INCLUDE=-Iinclude
LED_LIB_DIR=lib
LED_LIB_FILENAME=libSSD1306.a
LED_LIB=$(LED_LIB_DIR)/$(LED_LIB_FILENAME)
LED_OBJ_LIB=$(OBJS_DIR)/led.a

WIRINGPI_LIBS=-lwiringPi

CONF_DIR=conf
SCRIPTS=init.sh ovitz.sh
SCRIPTS_DIR=scripts
SCRIPTS_HOME_DIR=$(HOME)/rc.scripts

LOCK_DIR=lock

ALL_DIRS=$(OBJS_DIR) $(SCRIPTS_HOME_DIR) $(LOCK_DIR) 

all: init $(DEST) $(WRAPPER_DEST)

$(DEST): $(OBJS)
	$(CC) $(CFLAGS) $(OPENCV_LIBS) $(PICAMERA_LIBS) $(WIRINGPI_LIBS) $(OBJS) $(LED_OBJ_LIB) -o $(DEST)

$(WRAPPER_DEST): $(WRAPPER_OBJS)
	$(CC) $(CFLAGS) $(WIRINGPI_LIBS) $(WRAPPER_OBJS) $(LED_OBJ_LIB) -o $(WRAPPER_DEST)

$(OBJS_DIR)/start.o:
	$(CC) $(CFLAGS) $(LED_INCLUDE) $(WIRINGPI_LIBS) -c $(SRCS_DIR)/start.cpp -o $(OBJS_DIR)/start.o

$(OBJS_DIR)/main.o:
	$(CC) $(CFLAGS) $(LED_INCLUDE) $(PICAMERA_LIBS) -c $(SRCS_DIR)/main.cpp -o $(OBJS_DIR)/main.o

$(OBJS_DIR)/image.o:
	$(CC) $(CFLAGS) $(OPENCV_LIBS) -c $(SRCS_DIR)/image.cpp -o $(OBJS_DIR)/image.o

$(OBJS_DIR)/capture.o:
	$(CC) $(CFLAGS) $(LED_INCLUDE) $(OPENCV_LIBS) $(PICAMERA_LIBS) $(WIRINGPI_LIBS) -c $(SRCS_DIR)/capture.cpp -o $(OBJS_DIR)/capture.o

$(OBJS_DIR)/oled.o:
	$(CC) $(CFLAGS) $(LED_INCLUDE) -c $(SRCS_DIR)/oled.cpp -o $(OBJS_DIR)/oled.o
	cp $(LED_LIB) $(LED_OBJ_LIB)
	ar crs $(LED_OBJ_LIB) $(OBJS_DIR)/oled.o

$(OBJS_DIR)/util.o:
	$(CC) -c $(SRCS_DIR)/util.cpp -o $(OBJS_DIR)/util.o

init:
	make clean
	mkdir $(ALL_DIRS)
	touch $(LOCK_DIR)/window_avail
	sudo chmod 755 $(SCRIPTS_DIR)/init.sh $(SCRIPTS_DIR)/ovitz.sh
	cp $(SCRIPTS_DIR)/init.sh $(SCRIPTS_DIR)/ovitz.sh $(SCRIPTS_HOME_DIR)

clean:
	rm -f $(DEST)
	rm -f $(WRAPPER_DEST)
	rm -f $(LED_OBJ_LIB)
	sudo rm -rf $(ALL_DIRS)
