#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define __USE_MISC
#include <math.h>

#define MAX_TIME 1439

#define CLAMP(val, min, max) ((val) > (max) ? (max) : ((val) < (min) ? (min) : (val)))
#define TO_TIME(time) (((time) - ((time) % 60)) / 60), ((time) % 60)

typedef struct WethearStatus {
	double temp;
	double pressure;
	double radiative_heat;
	double humidity;
	double wind_effect;
	unsigned short int curr_time;
} WethearStatus;

static const double pressure_coeff       = 0.0536432;
static const double radiative_heat_coeff = 0.9646232;
static const double humidity_coeff       = 0.0713433532;
static const double wind_effect_coeff    = 0.001357232;

static double max_radiative_heat = 0;
static unsigned short int t_sunrise = 0;
static unsigned short int t_sunset = 0;

void init_heat_const(void) {
	max_radiative_heat = rand() / (double) RAND_MAX * 10;
	t_sunrise = rand() / 500000;
	t_sunrise = CLAMP(t_sunrise, 60 * 5, 60 * 7);
	t_sunset = rand() / 500000;
	t_sunset = CLAMP(t_sunset, 60 * 16, 60 * 21);
	printf("max_radiative_heat: %lf, t_sunrise: %02u:%02u, t_sunset: %02u:%02u\n", max_radiative_heat, TO_TIME(t_sunrise), TO_TIME(t_sunset));
	return;
}

double pressure_change(double pressure, unsigned int delta) {
	double diff = 0.0;
	for (unsigned int i = 0; i < delta / 15; ++i) diff += (((rand() % 2 ? -1 : 1)) * (rand() / (double) RAND_MAX));
	return CLAMP(pressure + diff, 0.0, 10.0);
}

double humidity_change(double humidity, unsigned int delta) {
	double diff = 0.0;
	for (unsigned int i = 0; i < delta; ++i) diff += (((rand() % 2 ? -1 : 1)) * (rand() / (double) RAND_MAX)) / 10.0;
	return CLAMP(humidity + diff, 0.0, 1.0);
}

double radiative_heat_change(unsigned int delta_time, unsigned short int curr_time) {
	return max_radiative_heat * sin((M_PI * (curr_time + delta_time - t_sunrise)) / (t_sunset - t_sunrise));
}

double wind_effect_change(double wind_effect, unsigned int delta) {
	double diff = 0.0;
	for (unsigned int i = 0; i < delta / 15; ++i) diff += (((rand() % 2 ? -1 : 1)) * (rand() / (double) RAND_MAX));
	return CLAMP(wind_effect + diff, -100.0, 100.0);
}

double temp_change(WethearStatus* weather, unsigned int delta_time) {	
	weather -> pressure = pressure_change(weather -> pressure, delta_time);
	weather -> humidity = humidity_change(weather -> humidity, delta_time);
	weather -> radiative_heat = radiative_heat_change(delta_time, weather -> curr_time);
	weather -> wind_effect = wind_effect_change(weather -> wind_effect, delta_time);
	weather -> curr_time = (weather -> curr_time + delta_time) % MAX_TIME;
	
	return (
			weather -> temp + 
			pressure_coeff * weather -> pressure + 
			radiative_heat_coeff * weather -> radiative_heat + 
			humidity_coeff * weather -> humidity + 
			wind_effect_coeff * weather -> wind_effect
		);
}

int main(void) {
	srand(time(0));
	init_heat_const();

	WethearStatus weather = {0};
	weather.temp = 10;
	weather.pressure = (rand() / (double) RAND_MAX);
	weather.humidity = (rand() / (double) RAND_MAX);
	weather.wind_effect = (rand() / (double) RAND_MAX) * 10;
	weather.curr_time = rand() / 500000 % MAX_TIME;
	printf("Curr Time: %02u:%02u - %u\n", TO_TIME(weather.curr_time), weather.curr_time);

	double original_temp = weather.temp;
	unsigned short int original_time = weather.curr_time;
	unsigned int delta_time = 15;
	unsigned int n = 10;

	for (unsigned int i = 0; i < n; ++i) {
		double new_temp = temp_change(&weather, delta_time);
		printf("The temperature variation: %.1lf -> %.1lf (over %02u:%02u hours).\n", weather.temp, new_temp, TO_TIME(delta_time));
		weather.temp = new_temp;
	}

	printf("The general temperature variation: %.1lf -> %.1lf (between %02u:%02u -> %02u:%02u).\n", original_temp, weather.temp, TO_TIME(original_time), TO_TIME(weather.curr_time));

	return 0;
}
