#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <TROOT.h>
#include <TApplication.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TStyle.h>
#include <TH1.h>

#include "binfile.h"

#define TBIN     0.4 // time bin size
#define BINPERNS 2.5 // 1/TBIN
#define FOURLOG10 9.2103404

int main(int argc,char **argv)
{
  TH1D         *likehis;

  short    int bgfit;
  unsigned int nlike,*offset,*nneg,*lmax;
  unsigned int *loglike,*time_histo;
  float        *qminmax,*pdf,*pdf_sum,*pdf_max;
  int          nbin,s0,sx4,sx2,det;
  float        *tof,*dir,*like,*sig;

  unsigned int *distp;
  int          i,*sizes,*numbers;
  void         **starts;
  int          k,set,part;

  // **********************************************
  // load timing distribution from disk
  // **********************************************
  // ********************** load pdfs from file **********************
  printf("Loading pdfs...\n");
  binfile      bf(argv[1],'r');

  if (bf.read(sizes,numbers,starts)!=2)
    {
      printf("Not a valid likelihood binary file!\n");
      exit(1);
    }
  //printf("numbers=%d %d\n",numbers[0],numbers[1]);
  nlike=numbers[0]/3;
  bgfit=((int *)(starts[0]))[nlike-1];
  //printf("nlike=%d, bgfit=%d\n",nlike,bgfit);
  if (bgfit) printf("Turn on background fitting\n");
  offset=(unsigned int *)(starts[0]);
  //printf("starts[0]=%d %d %d\n",offset[0],offset[1],offset[2]);
  //for(i=0; i<numbers[1]; i++)
  //printf("%u\n",((unsigned int *)starts[1])[i]);
  offset[nlike-1]=numbers[1];
  nneg=offset+nlike;
  qminmax=(float *)(starts[0])+2*nlike;
  //printf("qminmax=%f\n",qminmax[0]);

  // create arrays
  loglike=new unsigned int[5*offset[nlike-1]];
  pdf=new float[5*offset[nlike-1]];
  pdf_sum=new float[5*offset[nlike-1]];
  lmax=new unsigned int[nlike];
  pdf_max=new float[2*nlike];
  sig=new float[nlike];
  distp=(unsigned int *)(starts[1]);
  for(i=0; i<numbers[1]; i++) loglike[i]=distp[i];
  delete(distp);
  delete(numbers);
  delete(sizes);
  delete(starts);

  // integrate distributions
  part=offset[nlike-1];

  for(set=i=0; set<(int)nlike; set++)
    {
      for(lmax[set]=loglike[offset[set]-1]; i<(int)offset[set]-1; i++)
	{
	  if (loglike[i]>lmax[set])
	    lmax[set]=loglike[i];
	  for(k=1; k<5; k++)
	    loglike[k*part+i]=((5-k)*loglike[i]+k*loglike[i+1])/5;
	}
      for(k=1; k<5; k++)
	loglike[k*part+i]=(5-k)*loglike[i]/5;
      i++;
    }
  // calculate the log(likelihood) values from the integers
  for(i=0; i<5*part; i++)
    pdf[i]=exp(FOURLOG10*(loglike[i]*1E-5-1));
  // sum up the log(likelihood) for each charge bin
  for(set=i=0; set<(int)nlike; set++)
    {
      pdf_sum[i]=pdf[i];
      for(k=1; k<5; k++)
	pdf_sum[k*part+i]=pdf_sum[(k-1)*part+i]+pdf[k*part+i];
      for(i++; i<(int)offset[set]; i++)
	{
	  pdf_sum[i]=pdf[i]+pdf_sum[4*part+i-1];
	  for(k=1; k<5; k++)
	    pdf_sum[k*part+i]=pdf[k*part+i]+pdf_sum[(k-1)*part+i];
	}
      pdf_max[nlike+set]=exp(FOURLOG10*(1e-5*lmax[set]-1));
    }
  for(set=0; set<(int)nlike-1; set++)
    printf("pdf for %5.2fpe<=q<%5.2fpe: zero at %d total=%fns\n",
	   qminmax[set],qminmax[set+1],nneg[set],
	   0.2*TBIN*pdf_sum[4*part+offset[set]-1]);
  printf("pdf for %5.2fpe<=q: zero at %d total=%fns\n",
	 qminmax[set],nneg[set],0.2*TBIN*pdf_sum[4*part+offset[set]-1]);

  // ********** define constants for the log L peak fitting **********
  nbin=3;
  s0=2*nbin+1;
  sx2=nbin*(nbin+1)*s0/3;
  sx4=nbin*(nbin*nbin*nbin+
	    (2*nbin*nbin*nbin*nbin+3)/5)+
            2*nbin*(nbin*nbin-1)/3;
  det=sx4*s0-sx2*sx2;

  // display
  TApplication showit("showit",&argc,argv);
  TCanvas      showcanvas("showcanvas","BONSAI Likelihood");

  showcanvas.cd();
  for(set=0; set<(int)nlike; set++)
    {
      likehis=new TH1D("likehis","BONSAI Likelihood",offset[set],
		      -((int)nneg[set])*TBIN,
		       ((int) offset[set]-(int) nneg[set])*TBIN);
      //for(i=0; i<offset[set]; i++)
      //printf("%d %lf\n",i+1,exp(FOURLOG10*(1e-5*loglike[i]-1)));
      for(i=0; i<offset[set]; i++)
	  likehis->SetBinContent(i+1,exp(FOURLOG10*(1e-5*loglike[i]-1)));
      likehis->Draw("L");
      gPad->SetTicks(1,1);
      gPad->SetLogy();
      gStyle->SetOptStat(0);
      showcanvas.Update();
    }
  showit.Run();
}
