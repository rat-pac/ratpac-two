#ifndef FITQUALITY
#define FITQUALITY
#include <math.h>
#include <vector>

// *************************************************************
// * dummy class to provide function to be maximized           *
// *************************************************************
class fitquality
{
  float rmax,rmax2,zmax,dwall; // maximum allowed x^2+y^2 and |z|
  float worst;
  std::vector<float> qualities;

 protected:
// *************************************************************
// * set worst ever quality                                    *
// *************************************************************
  inline void set_worst(float q)
    {
      worst=q;
    };
// *************************************************************
// * record all fit vertices and fit qualities                 *
// *************************************************************
  inline void add_fit(float *v,float q)
    {
      qualities.push_back(*v);
      qualities.push_back(v[1]);
      qualities.push_back(v[2]);
      qualities.push_back(q);
    };
  inline void reset_fits(void)
    {
      qualities.clear();
    };
// *************************************************************
// * check if a quality is worse than the worst ever           *
// *************************************************************
  inline void check_worst(float q)
    {
      if (q<worst) worst=q;
    };

 public:
// *************************************************************
// * define maximum x^2+y^2 and |z|
// *************************************************************
  inline fitquality(float r,float z,float dw)
    {
      dwall=dw;
      rmax=r-dwall;
      zmax=z-dwall;
      rmax2=rmax*rmax;
      worst=-1e20;
    }
// *************************************************************
// * determine, if pos is inside the fitting volume            *
// *************************************************************
  inline short int fit_volume(float *pos)
    {
      if (pos[2]<-zmax) return(0);
      if (pos[2]>zmax) return(0);
      return(pos[0]*pos[0]+pos[1]*pos[1]<=rmax2);
    }
// *************************************************************
// * Determine, if pos is in fitting volume; if it isn't find  *
// * point corrected, that is at the boundary                  *
// *************************************************************
  inline short int fit_volume(float *pos,float *corrected,float &rfac)
    {
      // calculate radius^2
      rfac=pos[0]*pos[0]+pos[1]*pos[1];
      if (rfac>rmax2) // if >maximum radius^2, determine shrink factor
	{             // and shrink
	  rfac=0.99995*rmax/sqrt(rfac);
	  corrected[0]=pos[0]*rfac;
	  corrected[1]=pos[1]*rfac;
	  // bound z position between -zmax and zmax and quit
	  if (pos[2]<-zmax)
	    corrected[2]=-zmax;
	  else if (pos[2]>zmax)
	    corrected[2]=zmax;
	  else corrected[2]=pos[2];
	  return(0);
	}
      // copy x and y position and bound z position between -zmax and zmax
      corrected[0]=pos[0];
      corrected[1]=pos[1];
      if (pos[2]<-zmax)
	{
	  corrected[2]=-zmax;
	  return(0);
	}
      if (pos[2]>zmax)
	{
	  corrected[2]=zmax;
	  return(0);
	}
      return(1);
    }
  inline virtual float quality(float *v)
    {
      return(-1e20);
    };
  inline virtual void check_around(float *vertex,float *result,
				   float radius,float *q,int &max)
    {
      max=-1;
      return;
    };
  inline virtual char ncheck(void)
    {
      return(0);
    }
  inline virtual void interpolate(float *vertex,
				  float radius,float *q,float *inter)
    {
    };
  inline virtual int nresult(void)
    {
      return(0);
    };
  inline virtual void get_result(float *r)
    {
      return;
    };
  inline virtual void set_result(float *r)
    {
      return;
    };
  inline virtual void set_branch(short int point)
    {
      return;
    };
  inline float worstquality(void)
    {
      return(worst);
    };
  inline float returnrvol(void)
    {
      return(rmax+dwall);
    };
  inline float returnzvol(void)
    {
      return(zmax+dwall);
    };
  inline float walldist(float x,float y,float z)
    {
      float rwall=rmax+dwall-sqrt(x*x+y*y);
      float zwall=zmax+dwall-fabs(z);

      if (rwall<zwall) return(rwall); else return(zwall);
    };
// return average fit quality excluding a sphere of given center and radius 
inline int average_quality(float &ave,float *center,float radius)
  {
    int nq;
    float dx,dy,dz;
    ave = 0.;

    if (qualities.size()<4) return(0);
    for(int i=nq=0; i<qualities.size(); i+=4)
      {
	if (radius<=0)
	  {
	    ave+=qualities[i+3];
	    ++nq;
	    continue;
	  }
	dx=qualities[i]-center[0];
	dy=qualities[i+1]-center[1];
	dz=qualities[i+2]-center[2];
	if (dx*dx+dy*dy+dz*dz>radius*radius)
	  {
	    ave+=qualities[i+3];
	    ++nq;
	  }
      }
    if (nq<1) return(nq);
    ave/=nq;
    return(nq);
  }
};
#endif
