#ifndef ARIADNE
#define ARIADNE
class ariadne
{
  float direction[3],goodness,max_length,quality,cos_scat;
  int   nrscat,nrdir;
  char *active;
  float *directions,*clusdir;

  void cone_intersect(float cos_theta,
		      float *axis1,float *axis2,float *dir1,float *dir2,
		      int &solution);
  void find_largest_cluster(float &limit,int &max_index,float &maxmag,int &nradd,
			  int &clus_index);
  void refine_cluster(float &limit,int &max_index,float &maxmag,int &nradd,
		    int &clus_index);
  void final_scan(float limit,int &max_index,float &maxmag,int &nradd,int &clus_index);
  void sum_dir(float *sum,int &nradd,float *direc,float cos_min);

public:
  ariadne(float *vertex,int nrhits,float *position,float cos_theta);
  ~ariadne(void);
  void fit(void);
  inline float dir_goodness(void)
  { return(goodness); }
  inline float dir_quality(void)
  { return(quality); }
  inline float cos_scat_angle(void)
  { return(cos_scat); }
  inline int nr_scat(void)
  { return(nrscat); }
  inline void dir(float *d)
  { d[0]=direction[0]; d[1]=direction[1]; d[2]=direction[2];}
};
#endif
