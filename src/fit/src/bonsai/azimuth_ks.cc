#include <math.h>
#include <stdio.h>
using namespace std;
#include <RAT/BONSAI/azimuth_ks.h>

#include <algorithm>

#define PI 3.14159265359

/**************************************************************************
 * KS test of azimuthal symmetry of the Cherenkonv cone                   *
 **************************************************************************/
float azimuth_ks(int nhit, float *hits, float *vertex, float *dir) {
  float *phi;
  float cos_phi, sin_phi;
  float rot_xx, rot_xy, rot_xz, rot_yx, rot_yy;
  float dx, dy, dz, rad, expected, dev, mi, ma;
  int hit;

  // make rotation matrix using (theta_hat,phi_hat,r_hat) as rows from polar
  // coordinates
  rad = dir[0] * dir[0] + dir[1] * dir[1];
  if (rad > 0) {
    rad = sqrt(rad);
    cos_phi = dir[0] / rad;
    sin_phi = dir[1] / rad;
    rot_xx = dir[2] * cos_phi;
    rot_xy = dir[2] * sin_phi;
    // better than 1-dir[2]^2 numerically
    rot_xz = -sqrt((1 + dir[2]) * (1 - dir[2]));
    rot_yx = -sin_phi;
    rot_yy = cos_phi;
  } else {
    rot_xy = rot_xz = rot_yx = 0;
    rot_yy = 1;
    if (dir[2] > 0)
      rot_xx = 1;
    else
      rot_xx = -1;
  }

  // calculate all azimuth values
  phi = new float[nhit];
  for (hit = 0; hit < nhit; hit++) {
    // calculate normalized hit vectors
    dx = hits[3 * hit] - vertex[0];
    dy = hits[3 * hit + 1] - vertex[1];
    dz = hits[3 * hit + 2] - vertex[2];
    rad = sqrt(dx * dx + dy * dy + dz * dz);
    dx /= rad;
    dy /= rad;
    dz /= rad;
    phi[hit] = atan2(rot_yx * dx + rot_yy * dy, rot_xx * dx + rot_xy * dy + rot_xz * dz);
  }
  // sort azimuth values
  sort(phi, phi + nhit);
  expected = 2 * PI / nhit;  // expected spacing of azimuthal values

  // find maximum and minimum deviation from expected azimuthal values
  mi = +9999;
  ma = -9999;
  for (hit = 0; hit < nhit; hit++) {
    dev = phi[hit] - hit * expected;
    if (dev < mi) mi = dev;
    if (dev > ma) ma = dev;
  }
  delete[] phi;
  // KS value is difference divided by largest possible difference (2pi)
  return ((ma - mi) / (2 * PI));
}
