#ifndef _ZEPHYROS_H_
#define _ZEPHYROS_H_

#include <stdio.h>
#include <stdlib.h>

#define __USE_MISC
#include <math.h>

#ifdef _WIN32
	#define EXPORT __declspec(dllexport)
#else
	#define EXPORT
#endif //_WIN32

#define MAX_TIME 1439 // 23:59

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(val, min, max) ((val) > (max) ? (max) : ((val) < (min) ? (min) : (val)))
#define TO_TIME(time) (((time) - ((time) % 60)) / 60), ((time) % 60)
#define MAGNITUDE(x, y) sqrt(((x) * (x)) + ((y) * (y)))

typedef struct WeatherStatus {
	double temp;
	double pressure;
	double humidity;
	double wind_velocity;
	uint8_t wind_direction;
	uint16_t curr_time;
} WeatherStatus;

static double rk_4(double k1, double h) {
	double k2 = k1 + h * k1 / 2;
	double k3 = k1 + h * k2 / 2;
	double k4 = k1 + h * k3;
	return k1 + (h / 6) * (k1 + 2 * k2 + 2 * k3 + k4);
}

static int rand_int(int min, int max) {
	int r = rand();
	return CLAMP(r, min, max); 
}

// Constants, to play with the toy model
#define GAMMA   0.00000000713433532
#define DELTA   0.0000000713433532
#define M       12000
#define BETA_1  0.0000000713433532
#define BETA_2  0.00000713433532
#define BETA_3  0.00000713433532
#define BETA_4  0.0000713433532
#define ALPHA_1 0.000000713433532
#define ALPHA_2 0.0000713433532
#define A       0.00000713433532
#define EPSILON 0.00000713433532
#define SIGMA   0.00000713433532
#define LAMBDA  0.00713433532

#define H_SAT(t) (0.8 * exp(0.06 * (t)))
#define C(h, temp) MAX(0, H_SAT(temp) - h)
#define Q_IN(t) (A * sin((2 * M_PI * (t)) / 86400))
#define Q_OUT(temp) (EPSILON * SIGMA * (temp) * (temp) * (temp) * (temp))

EXPORT WeatherStatus simulate_weather(WeatherStatus* weather_status, uint64_t seed, uint8_t hours) {
	srand(seed);

	const double t_ref = rand_int(10, 20); 
	const double p_ref = rand_int(900, 1500); 
	const double h_env = rand_int(50, 75); 
	const double p_env = rand_int(900, 1500); 

	// Perform 40 RK4 steps with step size 0.025 to simulate an hour
	for (unsigned int i = 0; i < 40 * hours; ++i) {
		double temp = weather_status -> temp - GAMMA * MAGNITUDE(weather_status -> wind_direction, weather_status -> wind_velocity) * GAMMA * (weather_status -> temp - t_ref)
		              + DELTA * C(weather_status -> humidity, weather_status -> temp)
					  + BETA_1 * (weather_status -> pressure - p_ref) + (Q_IN(i) - Q_OUT(weather_status -> temp)) / M;
		weather_status -> temp = rk_4(temp, 0.00025);

		double pressure = weather_status -> pressure - GAMMA * MAGNITUDE(weather_status -> wind_direction, weather_status -> wind_velocity)
		              * GAMMA * (weather_status -> pressure - p_env) + ALPHA_1 * (weather_status -> temp - t_ref);
		weather_status -> pressure = rk_4(pressure, 0.00025);

		double humidity = weather_status -> humidity - GAMMA * MAGNITUDE(weather_status -> wind_direction, weather_status -> wind_velocity)
		              * GAMMA * (weather_status -> humidity - h_env) - BETA_2 * C(weather_status -> humidity, weather_status -> temp)
					  + BETA_3 * (weather_status -> pressure - p_ref);
		weather_status -> humidity = fmodf(rk_4(humidity, 0.0025), 100.0);

		double wind_direction = weather_status -> wind_direction - LAMBDA * weather_status -> wind_direction
		              + ALPHA_2 * (weather_status -> temp * t_ref)
					  + BETA_4 * (weather_status -> pressure - p_ref);
		weather_status -> wind_direction = fmod(rk_4(wind_direction, 0.0025), 360.0);

		double wind_velocity = weather_status -> wind_velocity - LAMBDA * weather_status -> wind_velocity
		              + ALPHA_2 * (weather_status -> temp * t_ref)
					  + BETA_4 * (weather_status -> pressure - p_ref);
		weather_status -> wind_velocity = rk_4(wind_velocity, 0.0025);
	}

	return *weather_status;
}

#endif //_ZEPHYROS_H_
