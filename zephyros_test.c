#include <stdio.h>
#include <stdlib.h>

#include "zephyros.h"

int main(void) {
	init_heat_const();

	WethearStatus weather = {0};
	weather.temp = 10;
	weather.pressure = (rand() / (double) RAND_MAX);
	weather.humidity = (rand() / (double) RAND_MAX);
	weather.wind_effect = (rand() / (double) RAND_MAX) * 10;
	weather.curr_time = rand() / 500000 % MAX_TIME;

	double original_temp = weather.temp;
	unsigned short int original_time = weather.curr_time;
	unsigned int delta_time = 15;
	unsigned int n = 20;

	for (unsigned int i = 0; i < n; ++i) {
		double new_temp = temp_change(&weather, delta_time);
		//printf("The temperature variation: %.1lf -> %.1lf (over %02u:%02u hours).\n", weather.temp, new_temp, TO_TIME(delta_time));
		weather.temp = new_temp;
	}

	printf("The general temperature variation: %.1lf -> %.1lf (between %02u:%02u -> %02u:%02u).\n", original_temp, weather.temp, TO_TIME(original_time), TO_TIME(weather.curr_time));

	return 0;
}

