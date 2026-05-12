#ifndef _KALMAN_FILTER_H
#define _KALMAN_FILTER_H

#include <stdint.h>

#define REFRESH_RATE 0.005 // 5ms

typedef struct angle_calculations_struct {
  double angle;
  double gyro_y;
  double q_bias;
  double angle_err;
  double q_angle;
  double q_gyro;
  double r_angle;
  double dt;
  uint8_t c_0;
  double pct_0, pct_1, e;
  double k_0, k_1, t_0, t_1;
  double pdot[4];
  double pp[2][2];
} angle_calculations;

double filter(double accel, double gyro, angle_calculations *calc);

#endif
