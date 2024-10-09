#include <RAT/BONSAI/ariadne.h>
#include <math.h>
#include <stdio.h>

#define COS_CUT 0.88    // angular search window
#define COS_CUT2 0.65   // limit to distinguish alternate directions
#define CUT_FRAC1 0.80  // first cut fraction
#define CUT_FRAC2 0.95  // second cut fraction

/***********************************************************************
 *    -------------------------------------                            */
ariadne::ariadne(float *vertex, int nrhits, float *positions, float cos_theta)
/*    -------------------------------------                            *
 *                                                                     *
 *    (Purpose)                                                        *
 *      find the direction from a labyrinth of multiply scattered      *
 *      PMT hits                                                       *
 *                                                                     *
 *    (Input)                                                          *
 *      vertex        -> found vertex of the event                     *
 *      nrhits        -> number of hit PMTs                            *
 *      positions     -> positions of the PMTs                         *
 *                                                                     *
 *    (Output)                                                         *
 *      direction     -> direction of the event                        *
 *      goodness      -> goodness of the fit                           *
 *      quality       -> ratio of used directions                      *
 *      nrscat        -> number of possible directions                 *
 *      cos_scat      -> cos of maximum scattering angle               *
 *                                                                     *
 *    (Creation Date and Author)                                       *
 *      1998.02.12 Michael Smy   Version 1.0: creation                 *
 *      1998.04.10 Michael Smy   Version 1.1: use multi-pass approach  *
 *      1998.04.13 Michael Smy   Version 1.2: simplify array structure *
 *      1998.04.22 Michael Smy   Version 1.3: add mult. scat. analysis *
 *      1998.05.05 Michael Smy   Version 2.0: smooth out direction bias*
 *      2018.12.18 Michael Smy   Version 3.0: translate to C           *
 *                                                                     *
 ----------------------------------------------------------------------*/
{
  /*---------------------------------------------------------------------*
        (constants)                                                      *
   ----------------------------------------------------------------------*
                                                                         *
        (local variables)                                                */
  float distance, *hit_vectors;  //*
  int row, column, i, sol;       //*
                                 /*                                                                     *
                                 ***********************************************************************/
  goodness = -1;
  quality = 0;
  direction[0] = direction[1] = direction[2] = -2;
  cos_scat = 2;
  nrscat = -1;

  //-------------------------------------------------------------------
  // get all the direction hit_vectors from the vertex to the PMTs
  //-------------------------------------------------------------------
  hit_vectors = new float[nrhits * 3];
  for (row = 0; row < nrhits; row++) {
    distance = 0;
    for (i = 0; i < 3; i++) {
      hit_vectors[3 * row + i] = positions[3 * row + i] - vertex[i];
      distance += hit_vectors[3 * row + i] * hit_vectors[3 * row + i];
    }
    distance = sqrt(distance);
    for (i = 0; i < 3; i++) hit_vectors[3 * row + i] /= distance;
  }

  /*-------------------------------------------------------------------
    find a pair of directions for each pair of PMTs
    -------------------------------------------------------------------*/
  max_length = 0.5 * nrhits * (nrhits + 1);  // set maximum possible length of sum vector
  nrdir = 0;
  directions = new float[nrhits * (nrhits + 1) * 4];
  for (row = 0; row < nrhits; row++)
    for (column = row + 1; column < nrhits; column++) {
      cone_intersect(cos_theta, hit_vectors + 3 * row, hit_vectors + 3 * column, directions + 4 * nrdir,
                     directions + 4 * (nrdir + 1), sol);
      if (sol == 2) {
        directions[4 * nrdir + 3] = 1;
        directions[4 * (nrdir + 1) + 3] = 1;
        nrdir += 2;
      }
    }
  delete[] (hit_vectors);
  active = NULL;
  clusdir = NULL;
}
ariadne::~ariadne(void) {
  delete[] directions;
  if (active != NULL) delete[] (active);
  if (clusdir != NULL) delete[] (clusdir);
}

/***********************************************************************
 *    -------------------------------------                            */
void ariadne::fit(void) {               //                                                                   *
                                        /*    -------------------------------------                            *
                                         *                                                                     *
                                         *    (Purpose)                                                        *
                                         *      performs the direction fit                                     *
                                         ----------------------------------------------------------------------*
                                         *                                                                     *
                                         *    (local variables)                                                */
  float tempdir[4], limit, maxmag;      //*
  int i, max_index, nradd, clus_index;  //*
                                        /*                                                                     *
                                        ***********************************************************************/

  goodness = -1;
  quality = 0;
  direction[0] = direction[1] = direction[2] = -2;
  cos_scat = 2;
  nrscat = -1;
  if (active == NULL) active = new char[nrdir];
  if (clusdir == NULL) clusdir = new float[4 * nrdir];
  find_largest_cluster(limit, max_index, maxmag, nradd, clus_index);
  if (clus_index == 0) return;
  limit = maxmag * CUT_FRAC1;
  refine_cluster(limit, max_index, maxmag, nradd, clus_index);
  if (max_index == -1) return;
  limit = maxmag * CUT_FRAC2;
  final_scan(limit, max_index, maxmag, nradd, clus_index);
  if (max_index == -1) return;

  goodness = clusdir[4 * (max_index - 1) + 3] / max_length;
  sum_dir(tempdir, nradd, clusdir + 4 * (max_index - 1), COS_CUT);
  quality = nradd / max_length;
  if (nradd < 3) return;
  for (i = 0; i < 3; i++) direction[i] = tempdir[i];
}

/***********************************************************************
 *    -------------------------------------                            */
void ariadne::cone_intersect(float cos_theta, float *axis1, float *axis2, float *dir1, float *dir2, int &solution)  //*
{  //                                                                   *
   /*    -------------------------------------                            *
    *                                                                     *
    *    (Purpose)                                                        *
    *      intersect two Cherenkov cones, if possible                     *
    *                                                                     *
    *    (Input)                                                          *
    *      axis1         -> normal vector along the axis of the 1st cone  *
    *      axis2         -> normal vector along the axis of the 2nd cone  *
    *                                                                     *
    *    (Output)                                                         *
    *      dir1          -> first possible intersection vector            *
    *      dir2          -> second possible intersection vector           *
    *      solution      -> number of solutions found (0 to 2)            *
    *                       3 solutions: axis1=axis2                      *
    *                                                                     *
    *    (Creation Date and Author)                                       *
    *      1998.02.12 Michael Smy   Version 1.0: creation                 *
    *      2018.12.18 Michael Smy   Version 2.0: translate to C           *
    *                                                                     *
    *---------------------------------------------------------------------*
    *---------------------------------------------------------------------*
    *     (constants)                                                     */
   /*--------------------------------------------------------------------*
    *---------------------------------------------------------------------*
    *    (local variables)                                                */
  double cos_alpha, ratio, sin_gamma2, cross[3];  //                       *
  double ax1[3], ax2[3], sin_alpha2;              //                                  *
  int i;                                          //                                                         *
  /*                                                                    *
  ***********************************************************************
  * Algorithm:cos(alpha)=axis1 dot axis2                                *
  *           cos(gamma)=cos(theta)/cos(alpha/2)(theta: Cherenkov angle)*
  *           b1=axis1+axis2 (normalized)                               *
  *           b2=axis1xaxis2 (normalized)                               *
  *           intersect=b1*cos(gamma)+-b2*sin(gamma)                    *
  ***********************************************************************/

  //===================================================================
  // Subroutine begin:
  //-------------------------------------------------------------------
  // find dot product and vector sum; convert to double precision
  cos_alpha = 0;
  for (i = 0; i < 3; i++) {
    ax1[i] = axis1[i];
    ax2[i] = axis2[i];
    dir1[i] = ax1[i] + ax2[i];
    cos_alpha += ax1[i] * ax2[i];
  }
  //-------------------------------------------------------------------
  // both vectors identical
  if (fabs(cos_alpha) == 1) {
    solution = 3;
    return;
  }

  //-------------------------------------------------------------------
  // ratio=cos(gamma)/||axis1+axis2||
  //      =cos(theta)/(cos(alpha/2)*sqrt(2*(1+cos(alpha)))
  //      =cos(theta)/sqrt(0.5*(1+cos(alpha)))*sqrt(2*(1+cos(alpha)))
  //      =cos(theta)/(1+cos(alpha))
  // sin(gamma)**2=1-cos(theta)**2/cos(alpha/2)**2
  //              =1-2*cos(theta)**2/(1+cos(alpha))
  //              =1-2*cos(theta)*ratio
  // no intersection possible --> return; also special case for one solution

  ratio = cos_theta / (1 + cos_alpha);
  sin_gamma2 = 1 - 2 * cos_theta * ratio;
  if (sin_gamma2 < 0) {
    solution = 0;
    return;
  }
  for (i = 0; i < 3; i++) {
    dir1[i] *= ratio;
    dir2[i] = dir1[i];
  }
  if (sin_gamma2 == 0) {
    solution = 1;
    return;
  }

  //-------------------------------------------------------------------
  // 2 solutions: for better precision, use
  // ||axis1xaxis2||**2=||axis1||**2||axis2||**2-cos(alpha)**2 instead of
  //                    1-cos(alpha)**2
  // this is important for very small alpha, where the
  //  second expression becomes numerically unstable
  solution = 2;
  sin_alpha2 =
      (ax1[0] * ax1[0] + ax1[1] * ax1[1] + ax1[2] * ax1[2]) * (ax2[0] * ax2[0] + ax2[1] * ax2[1] + ax2[2] * ax2[2]) -
      cos_alpha * cos_alpha;
  if (sin_alpha2 == 0) {
    solution = 3;
    return;
  }

  // now properly normalize the cross product
  ratio = sqrt(sin_gamma2 / sin_alpha2);
  cross[0] = ratio * (ax1[1] * ax2[2] - ax2[1] * ax1[2]);
  cross[1] = ratio * (ax1[2] * ax2[0] - ax2[2] * ax1[0]);
  cross[2] = ratio * (ax1[0] * ax2[1] - ax2[0] * ax1[1]);
  for (i = 0; i < 3; i++) {
    dir1[i] += cross[i];
    dir2[i] -= cross[i];
  }
  return;
}

void ariadne::find_largest_cluster(float &limit, int &max_index, float &maxmag, int &nradd, int &clus_index) {
  int row;

  for (row = 0; row < nrdir; row++) active[row] = 1;
  maxmag = 2.5;
  limit = 2.5;
  clus_index = 0;
  /*-------------------------------------------------------------------
    loop through direction and try to add as many directions
    as possible (with a large angualar window). don't loop through the
    directions already added
    -------------------------------------------------------------------*/
  for (row = 0; row < nrdir; row++)
    if (active[row]) {
      sum_dir(clusdir + clus_index * 4, nradd, directions + 4 * row, COS_CUT);
      /*-------------------------------------------------------------------
        store all directions that have at least 80% of the maximal goodness,
        find the maximal goodness */
      if (clusdir[clus_index * 4 + 3] < limit) continue;
      if (clusdir[clus_index * 4 + 3] > maxmag) {
        maxmag = clusdir[clus_index * 4 + 3];
        limit = maxmag * CUT_FRAC1;
        max_index = clus_index + 1;
      }
      ++clus_index;
    }
}

void ariadne::refine_cluster(float &limit, int &max_index, float &maxmag, int &nradd, int &clus_index) {
  int i;
  float cosine, tempdir[4];
  /*-------------------------------------------------------------------
    reprocess the best directions with successively smaller angualar windows*/

  maxmag = 1.5;
  for (i = 0; i < 3; i++) tempdir[i] = clusdir[4 * (max_index - 1) + i];
  cos_scat = 1;
  nrscat = 0;
  max_index = -1;
  for (i = 0; i < clus_index; i++)
    if (clusdir[3 + 4 * i] > limit) {
      cosine = clusdir[4 * i] * tempdir[0] + clusdir[4 * i + 1] * tempdir[1] + clusdir[4 * i + 2] * tempdir[2];
      if (cosine < COS_CUT2) {
        ++nrscat;
        if (cosine < cos_scat) cos_scat = cosine;
      }
      sum_dir(tempdir, nradd, clusdir + 4 * i, COS_CUT);
      if (tempdir[3] < maxmag * CUT_FRAC2)
        clusdir[4 * i + 3] = 0;
      else if (tempdir[3] > maxmag) {
        maxmag = tempdir[3];
        max_index = i + 1;
        clusdir[4 * i] = tempdir[0];
        clusdir[4 * i + 1] = tempdir[1];
        clusdir[4 * i + 2] = tempdir[2];
        clusdir[4 * i + 3] = maxmag;
      }
    } else
      clusdir[4 * i + 3] = 0;
}

void ariadne::final_scan(float limit, int &max_index, float &maxmag, int &nradd, int &clus_index) {
  float tempdir[4];
  int i;

  /*-------------------------------------------------------------------
    one last pass to improve the core resolution, calculate goodness
    and quality from the best direction found */
  maxmag = 1.5;
  max_index = -1;
  for (i = 0; i < clus_index; i++)
    if (clusdir[4 * i + 3] > limit) {
      sum_dir(tempdir, nradd, clusdir + 4 * i, COS_CUT);
      if (tempdir[3] > maxmag) {
        maxmag = tempdir[3];
        max_index = i + 1;
        clusdir[4 * i] = tempdir[0];
        clusdir[4 * i + 1] = tempdir[1];
        clusdir[4 * i + 2] = tempdir[2];
        clusdir[4 * i + 3] = maxmag;
      }
    } else
      clusdir[4 * i + 3] = 0;
}

/***********************************************************************
 *    -------------------------------------                            */
void ariadne::sum_dir(float *sum, int &nradd, float *direc, float cos_min)
/*    -------------------------------------                            *
 *                                                                     *
 *     (Purpose)                                                       *
 *       sum directions consistent with a given directions             *
 *                                                                     *
 *     (Input)                                                         *
 *       direc         -> direction around the sum is formed           *
 *       cos_min       -> cos of the maximum ang. deviation from direc *
 *       nrdir         -> nr of directions                             *
 *       COMMON BLOCK dirfitar:                                        *
 *       directions    -> array of all directions defined by hit pairs *
 *       active        -> tells, whether a direction can serve as seed *
 *                                                                     *
 *     (Output)                                                        *
 *       sum           -> the normalized sum of all selected directions*
 *                        the fourth componend gives the magnitude     *
 *       nradd         -> the number of directions selected            *
 *                                                                     *
 *    (Creation Date and Author)                                       *
 *       1998.02.12 Michael Smy   Version 1.0: creation                *
 *       1998.04.3  Michael Smy   Version 1.1: added mult-pass algor.  *
 *       2018.12.18 Michael Smy   Version 2.0: translated to C         *
 *                                                                     *
 ----------------------------------------------------------------------*/
{
  /*---------------------------------------------------------------------*
        (constants)                                                      *
   ----------------------------------------------------------------------*
   ----------------------------------------------------------------------*
        (local variables)                                                */
  int j, index;
  float norm;
  /***********************************************************************

   ===================================================================
    Subroutine begin:
   -------------------------------------------------------------------
    loop through all defined directions, form the scalar product with
    direc and add it, if eligible */

  // printf("start %lf %lf %lf %lf (%lf)\n",direc[0],direc[1],direc[2],direc[3],cos_min);
  nradd = 0;
  for (j = 0; j < 3; j++) sum[j] = 0;
  for (index = 0; index < nrdir; index++)
    if (direc[0] * directions[4 * index] + direc[1] * directions[4 * index + 1] + direc[2] * directions[4 * index + 2] >
        cos_min) {
      active[index] = 0;
      for (j = 0; j < 3; j++) sum[j] += directions[4 * index + 3] * directions[4 * index + j];
      ++nradd;
    }
  sum[3] = sqrt(sum[0] * sum[0] + sum[1] * sum[1] + sum[2] * sum[2]);
  norm = 1 / sum[3];
  for (j = 0; j < 3; j++) sum[j] *= norm;
  // printf("result %lf %lf %lf %lf\n",sum[0],sum[1],sum[2],sum[3]);
}
