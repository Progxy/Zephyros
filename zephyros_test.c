#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "zephyros.h"

int main(void) {
	WeatherStatus weather = {0};

	double old_temp = weather.temp;
	weather = simulate_weather(&weather, time(0), 1);
	printf("The temperature variation: %.1lf -> %.1lf (over %02u:%02u hours).\n", old_temp, weather.temp, TO_TIME(60));

	return 0;
}

