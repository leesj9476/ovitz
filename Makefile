CC=g++
CFLAGS=-g -Wall -Wpedantic -Wextra --std=c++11 -Wno-psabi
SRCS_DIR=src
OBJS_DIR=obj
OBJS=$(addprefix $(OBJS_DIR)/, main.o image.o oled.o util.o)
DEST=ovitz

OPENCV=`pkg-config opencv --cflags --libs`
OPENCV_LIBS=$(OPENCV)

SSD1306_INCLUDE=include
SSD1306_LIB_DIR=lib
SSD1306_LIB_FILENAME=libSSD1306.a
SSD1306_LIB=$(SSD1306_LIB_DIR)/$(SSD1306_LIB_FILENAME)

$(DEST): dummy $(OBJS_DIR) $(OBJS)
	$(CC) $(CFLAGS) $(OPENCV_LIBS) $(OBJS) $(SSD1306_LIB_FILENAME) -o $(DEST)

$(OBJS_DIR)/main.o:
	$(CC) $(CFLAGS) -I $(SSD1306_INCLUDE) -c $(SRCS_DIR)/main.cpp -o $(OBJS_DIR)/main.o

$(OBJS_DIR)/image.o:
	$(CC) $(CFLAGS) $(OPENCV_LIBS) -c $(SRCS_DIR)/image.cpp -o $(OBJS_DIR)/image.o

$(OBJS_DIR)/oled.o:
	$(CC) $(CFLAGS) -I $(SSD1306_INCLUDE) -c $(SRCS_DIR)/oled.cpp -o $(OBJS_DIR)/oled.o
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
	-rm -f $(OBJS)
	-rm -f $(SSD1306_LIB_FILENAME)
	-rm -rf $(OBJS_DIR)
