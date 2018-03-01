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

SSD1306_INCLUDE=-Iinclude
SSD1306_LIB_DIR=lib
SSD1306_LIB_FILENAME=libSSD1306.a
SSD1306_LIB=$(SSD1306_LIB_DIR)/$(SSD1306_LIB_FILENAME)

WIRINGPI_LIBS=-lwiringPi

CONF_DIR=conf
SCRIPTS=init.sh ovitz.sh
SCRIPTS_DIR=$(HOME)/rc.scripts

LOCK_DIR=lock

ALL_DIRS=$(OBJS_DIR) $(SCRIPTS_DIR) $(LOCK_DIR)

all: init $(DEST) $(WRAPPER_DEST)

$(DEST): $(OBJS)
	$(CC) $(CFLAGS) $(OPENCV_LIBS) $(PICAMERA_LIBS) $(WIRINGPI_LIBS) $(OBJS) $(SSD1306_LIB_FILENAME) -o $(DEST)

$(WRAPPER_DEST): $(WRAPPER_OBJS)
	$(CC) $(CFLAGS) $(WIRINGPI_LIBS) $(WRAPPER_OBJS) $(SSD1306_LIB_FILENAME) -o $(WRAPPER_DEST)

$(OBJS_DIR)/start.o:
	$(CC) $(CFLAGS) $(SSD1306_INCLUDE) $(WIRINGPI_LIBS) -c $(SRCS_DIR)/start.cpp -o $(OBJS_DIR)/start.o

$(OBJS_DIR)/main.o:
	$(CC) $(CFLAGS) $(SSD1306_INCLUDE) $(PICAMERA_LIBS) -c $(SRCS_DIR)/main.cpp -o $(OBJS_DIR)/main.o

$(OBJS_DIR)/image.o:
	$(CC) $(CFLAGS) $(OPENCV_LIBS) -c $(SRCS_DIR)/image.cpp -o $(OBJS_DIR)/image.o

$(OBJS_DIR)/capture.o:
	$(CC) $(CFLAGS) $(SSD1306_INCLUDE) $(OPENCV_LIBS) $(PICAMERA_LIBS) $(WIRINGPI_LIBS) -c $(SRCS_DIR)/capture.cpp -o $(OBJS_DIR)/capture.o

$(OBJS_DIR)/oled.o:
	$(CC) $(CFLAGS) $(SSD1306_INCLUDE) -c $(SRCS_DIR)/oled.cpp -o $(OBJS_DIR)/oled.o
	cp $(SSD1306_LIB) ./
	ar crs $(SSD1306_LIB_FILENAME) $(OBJS_DIR)/oled.o

$(OBJS_DIR)/util.o:
	$(CC) -c $(SRCS_DIR)/util.cpp -o $(OBJS_DIR)/util.o

init:
	make clean
	mkdir $(ALL_DIRS)
	touch $(LOCK_DIR)/window_avail
	sudo chmod 755 $(CONF_DIR)/init.sh $(CONF_DIR)/ovitz.sh
	cp $(CONF_DIR)/init.sh $(CONF_DIR)/ovitz.sh $(SCRIPTS_DIR)

clean:
	-rm -f $(DEST)
	-rm -f $(WRAPPER_DEST)
	-rm -f $(SSD1306_LIB_FILENAME)
	-rm -rf $(ALL_DIRS)
