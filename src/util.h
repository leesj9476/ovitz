#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>

#include "types.h"
#include "image.h"

typedef struct Options {
	Options();

	bool option[MAX_OPTION_NUM];

	std::string image_filename;
	double basic_distance;
	double focal;
	double pixel_size;
	double threshold_percent;
	double threshold_top_percent;
	int threshold_area;

	int pixel_max;
	int pixel_min;
	
} Options;

int getPointDistance(const Point_t &, const Point_t &);

void makeLensMatrix();

bool isInt(const char *, bool);
bool isFloat(const char *, bool);

Options parseSettingFile();

#endif
