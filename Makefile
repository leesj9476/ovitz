CC=g++
CFLAGS=-g -Wall -Wpedantic -Wextra --std=c++11
SRCS_DIR=src
OBJS_DIR=obj

OBJS=$(addprefix $(OBJS_DIR)/, main.o image.o capture.o oled.o util.o)
DEST=ovitz

WRAPPER_DEST=start

OPENCV=`pkg-config opencv --cflags --libs`
OPENCV_LIBS=$(OPENCV)

PICAMERA_LIBS=-lraspicam -lraspicam_cv

SSD1306_INCLUDE=-Iinclude
SSD1306_LIB_DIR=lib
SSD1306_LIB_FILENAME=libSSD1306.a
SSD1306_LIB=$(SSD1306_LIB_DIR)/$(SSD1306_LIB_FILENAME)

WIRINGPI_LIBS=-lwiringPi

all: dummy $(DEST) $(WRAPPER_DEST)

$(DEST): $(OBJS_DIR) $(OBJS)
	$(CC) $(CFLAGS) $(OPENCV_LIBS) $(PICAMERA_LIBS) $(WIRINGPI_LIBS) $(OBJS) $(SSD1306_LIB_FILENAME) -o $(DEST)

$(WRAPPER_DEST): $(OBJS_DIR)/start.o
	$(CC) $(CFLAGS) $(WIRINGPI_LIBS) $(OBJS_DIR)/start.o -o $(WRAPPER_DEST)

$(OBJS_DIR)/start.o:
	$(CC) $(CFLAGS) $(WIRINGPI_LIBS) -c $(SRCS_DIR)/start.cpp -o $(OBJS_DIR)/start.o

$(OBJS_DIR)/main.o:
	$(CC) $(CFLAGS) -c $(SRCS_DIR)/main.cpp -o $(OBJS_DIR)/main.o

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

$(OBJS_DIR):
	mkdir $(OBJS_DIR)

dummy:
	make clean

clean:
	-rm -f $(DEST)
	-rm -f $(WRAPPER_DEST)
	-rm -f $(OBJS)
	-rm -f $(SSD1306_LIB_FILENAME)
	-rm -rf $(OBJS_DIR)
	-rm -rf $(CAM_DIR)
