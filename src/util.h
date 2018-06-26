#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>

#include "types.h"

typedef struct Options {
	Options();

	bool option[MAX_OPTION_NUM];

	std::string input_image_file;
	std::string output_image_file;
	double basic_distance;
	double focal;
	double pixel_size;
	double threshold_from_avg_percent;
	double threshold_from_top_percent;
	int threshold_area;

	int pixel_max;
	int pixel_min;

	int delay_ms;
	
} Options;

typedef struct Point_t {
	Point_t(double, double, int = NONEXIST);
	Point_t(int = 0, int = 0, int = NONEXIST);

	Point_t operator=(const Point_t &);
	bool operator==(const Point_t &);

	int x;
	int y;

	double real_x;
	double real_y;

	// EXIST, NONEXIST
	int avail;
} Point_t;

typedef struct Vertex_t {
	Point_t v[4];
} Vertex_t;

std::ostream& operator<<(std::ostream &, const Point_t &);

int getPointDistance(const Point_t &, const Point_t &);

void makeLensMatrix();

bool isInt(const char *, bool);
bool isFloat(const char *, bool);

Options parseSettingFile();

void int_handler(int);

#endif
