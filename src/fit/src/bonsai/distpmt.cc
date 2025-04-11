#include <RAT/BONSAI/distpmt.h>
#include <math.h>
#include <stdio.h>

double distpmt(double *vertex, double *dir, double r, double z, double *wall) {
  double cos2th = 1 - dir[2] * dir[2];
  double xypar, zpar;

  if (dir[2] > 0)
    zpar = -(z + vertex[2]) / dir[2];
  else if (dir[2] < 0)
    zpar = (z - vertex[2]) / dir[2];
  else
    zpar = 10 * (r + z);
  // printf("zpar=%lf ",zpar);
  if (cos2th > 0) {
    double off = (vertex[0] * dir[0] + vertex[1] * dir[1]) / cos2th;

    xypar = -off - sqrt(off * off - (vertex[0] * vertex[0] + vertex[1] * vertex[1] - r * r) / cos2th);
    /*printf("off=%lf ",off);
    printf("xy wall %lf %lf (%lf) ",vertex[0]+xypar*dir[0],vertex[1]+xypar*dir[1],
           sqrt((vertex[0]+xypar*dir[0])*(vertex[0]+xypar*dir[0])+
           (vertex[1]+xypar*dir[1])*(vertex[1]+xypar*dir[1])));*/
  } else
    xypar = 10 * (r + z);
  // printf("xypar=%lf ",xypar);
  if (xypar > zpar) {
    wall[0] = vertex[0] + xypar * dir[0];
    wall[1] = vertex[1] + xypar * dir[1];
    wall[2] = vertex[2] + xypar * dir[2];
    // printf("choose xy");
    return (-xypar);
  }
  wall[0] = vertex[0] + zpar * dir[0];
  wall[1] = vertex[1] + zpar * dir[1];
  wall[2] = vertex[2] + zpar * dir[2];
  return (-zpar);
}
