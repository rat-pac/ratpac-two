// Generates an neutrino-elastic scattering event, based on the
// cross-section as function of neutrino energy and the electron's
// recoil energy.  Allow for variations in the weak mixing angle and
// the possibility of a neutrino magnetic moment
//
// J. Formaggio (UW) -02/09/2005

// Converted to Geant4+GLG4Sim+RAT by Bill Seligman (07-Feb-2006).
// I'm following the code structure of the IBD classes:
// RATVertexGen_ES handles the G4-related tasks of constructing an
// event, while this class deals with the physics of the
// cross-section.  Some of the code (the flux in particular) is copied
// from IBDgen.

// Time dependency added by Will Yeadon 21-Sep-2017
// Contact wyeadon1@sheffield.ac.uk

#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>

#include <G4ParticleDefinition.hh>
#include <G4ParticleTable.hh>
#include <G4ThreeVector.hh>
#include <RAT/DB.hh>
#include <RAT/Log.hh>
#include <RAT/SNgen.hh>
#include <RAT/SNgenMessenger.hh>
#include <Randomize.hh>
#include <cmath>

namespace RAT {

// WGS: Constants copied from various places to make the code work.
const double XMaxDefault = 1e-45;  // Reasonable minimum for x-section (cm^2).
const double GFERMI = 1.16639e-11 / CLHEP::MeV / CLHEP::MeV;
const double XcMeVtoCmsqrd = 0.389379292e-21;

// WGS: We have to start from some value of sin2theta; use the stanard-model
// value:
const double SNgen::WEAKANGLE = 0.2227;
const int SNgen::NTRIAL = 10000;

const double SNgen::IBDDEFAULT = 0.926;
const double SNgen::ESDEFAULT = 0.0295;
const double SNgen::CCDEFAULT = 0.003;
const double SNgen::ICCDEFAULT = 0.024;
const double SNgen::NCDEFAULT = 0.0175;
const int SNgen::MODELDEFAULT = 1;  // 1: livermore, 2: gkvm

TGraph *graphIBD;
TGraph *graphES;
TGraph *graphCC;
TGraph *graphICC;
TGraph *graphNC;
TGraph *graphINC;
TGraph *graphNCRate;

TFile *IBD_fe = new TFile("$RATSHARE/models/SNGen/nu_e_data_1_7.root");
TF1 *glum_e = (TF1 *)IBD_fe->Get("f1");
TF1 *gmene_e = (TF1 *)IBD_fe->Get("f3");
TF1 *galpha_e = (TF1 *)IBD_fe->Get("f2");
TFile *IBD_feb = new TFile("$RATSHARE/models/SNGen/nu_eb_data_1_7.root");
TF1 *glum_eb = (TF1 *)IBD_feb->Get("f1");
TF1 *gmene_eb = (TF1 *)IBD_feb->Get("f3");
TF1 *galpha_eb = (TF1 *)IBD_feb->Get("f2");
TFile *IBD_fx = new TFile("$RATSHARE/models/SNGen/nu_x_data_1_7_X.root");
TF1 *glum_x = (TF1 *)IBD_fx->Get("f1");
TF1 *gmene_x = (TF1 *)IBD_fx->Get("f3");
TF1 *galpha_x = (TF1 *)IBD_fx->Get("f2");

SNgen::SNgen() {
  // Initialize everything.
  Reset();

  // Create a messenger to allow the user to change some ES parameters.
  messenger = new SNgenMessenger(this);

  // Get parameters from database. Note that we get the flux from the
  // IBD values; we assume that the flux for ES is the same as the
  // flux for IBD.
  //        DBLinkPtr libd = DB::Get()->GetLink("SN_SPECTRUM");
  //
  //        Emin = libd->GetD("emin");
  //        Emax = libd->GetD("emax");
  // Flux function
  //        rmpflux.Set(libd->GetDArray("spec_e"),
  //        libd->GetDArray("spec_flux"));

  // Other useful numbers
  //        FluxMax = rmpflux(Emin);

  // Get the electron mass.
  //        G4ParticleDefinition* electron =
  //        G4ParticleTable::GetParticleTable()->FindParticle("e-");
  //        massElectron = electron->GetPDGMass();
}

SNgen::~SNgen() {
  // I compulsively delete unused pointers.
  if (messenger != 0) {
    delete messenger;
    messenger = 0;
  }
}

CLHEP::HepLorentzVector SNgen::GenerateEvent(const G4ThreeVector &theNeutrino) {
  //
  //  Check if the maximum throwing number has been set.
  //
  if (!GetNormFlag()) SetXSecMax(NTRIAL);
  double XSecNorm = GetXSecMax();

  // Throw values against a cross-section.
  bool passed = false;
  double E, Nu;

  while (!passed) {
    // Pick a random E and Nu.
    E = GetRandomNumber(0, 100);
    Nu = GetRandomNumber(0., E);

    // Decided whether to draw again based on relative cross-section.
    double XCtest = XSecNorm * FluxMax * GetRandomNumber(0., 1.);
    double XCWeight = GetXSec(E, Nu);
    double FluxWeight = rmpflux(E);
    passed = XCWeight * FluxWeight > XCtest;
  }

  //
  //  Calculate the neutrino pT and incoming angle
  //
  G4double pT = E * theNeutrino.perp() / theNeutrino.mag();
  G4double pTq = asin(pT / E);

  //
  //  Set kinematic variables from kinetic energy
  //
  G4double X = 1.;
  G4double Y = Nu / E;
  G4double Q2 = 2. * E * X * Y * massElectron;
  G4double sin2q = Q2 / (4. * E * (E - Nu));
  G4double theta_lab = 2. * asin(sqrt(sin2q)) + pTq;
  G4double phi = GetRandomNumber(0., 2. * M_PI);

  //
  //  Now that energy/Q2 is selected, write a momentum transfer 4-std::vector q.
  //
  G4double qp = sqrt(pow(Nu, 2) + Q2);
  CLHEP::HepLorentzVector qVector;
  qVector.setPx(qp * sin(theta_lab) * cos(phi));
  qVector.setPy(qp * sin(theta_lab) * sin(phi));
  qVector.setPz(qp * cos(theta_lab));
  qVector.setE(Nu);

  //
  // Let pe_new = pe - (pv - pv_new) == pe - q;
  //
  CLHEP::HepLorentzVector theElectron;
  theElectron.setE(massElectron);
  theElectron += qVector;

  return theElectron;
}

void SNgen::Reset() {
  XSecMax = 0.0;
  SetNormFlag(false);
  SetMixingAngle(WEAKANGLE);
  SetNeutrinoMoment(0.0);

  SetIBDAmplitude(IBDDEFAULT);
  SetESAmplitude(ESDEFAULT);
  SetCCAmplitude(CCDEFAULT);
  SetICCAmplitude(ICCDEFAULT);
  SetNCAmplitude(NCDEFAULT);
  SetModel(MODELDEFAULT);
}

void SNgen::SetIBDAmplitude(double IBDAm) {
  if ((IBDAm < 0.) || (IBDAm > 1.)) {
    warn << "Set your IBD Amplitude between 0 and 1." << newline;
    return;
  }
  IBDAmp = IBDAm;
}

void SNgen::SetESAmplitude(double ESAm) {
  if ((ESAm < 0.) || (ESAm > 1.)) {
    warn << "Set your ES Amplitude between 0 and 1." << newline;
    return;
  }
  ESAmp = ESAm;
}

void SNgen::SetCCAmplitude(double CCAm) {
  if ((CCAm < 0.) || (CCAm > 1.)) {
    warn << "Set your CC Amplitude between 0 and 1." << newline;
    return;
  }
  CCAmp = CCAm;
}

void SNgen::SetICCAmplitude(double ICCAm) {
  if ((ICCAm < 0.) || (ICCAm > 1.)) {
    warn << "Set your ICC Amplitude between 0 and 1." << newline;
    return;
  }
  ICCAmp = ICCAm;
}

void SNgen::SetNCAmplitude(double NCAm) {
  if ((NCAm < 0.) || (NCAm > 1.)) {
    warn << "Set your NC Amplitude between 0 and 1." << newline;
    return;
  }
  NCAmp = NCAm;
}

void SNgen::SetModel(double ModelTm) {
  if ((ModelTm < 1.) || (ModelTm > 2.)) {
    warn << "Set your model between 1 (livermore) and 2. (gvkm)" << newline;
    return;
  }
  ModelTmp = ModelTm;
}

void SNgen::Show() {
  info << "Elastic Scatteing Settings:" << newline;
  info << "Weak Mixing Angle (sinsq(ThetaW)):" << GetMixingAngle() << newline;
  info << "Neutrino Magnetic Moment: " << GetMagneticMoment() << newline;
}

void SNgen::SetMixingAngle(double sin2thw) {
  if ((sin2thw < 0.) || (sin2thw > 1.)) {
    warn << "Error in value setting." << newline;
    return;
  }
  SinSqThetaW = sin2thw;
}

void SNgen::SetNeutrinoMoment(double vMu) {
  if (vMu < 0.) {
    warn << "Error in value setting." << newline;
    return;
  }
  MagneticMoment = vMu;
}

double SNgen::GetXSec(double Enu, double T) {
  double XC = 0.;

  //  Set up constants for cross-section scale.
  double XCunits = XcMeVtoCmsqrd;
  double Sigma0 = GFERMI * GFERMI * massElectron / (2. * M_PI);

  //  Set up weak mixing parameters & neutrino magnetic moment.
  double sin2thw = GetMixingAngle();
  double vMu = GetMagneticMoment();
  double g_v = 2. * sin2thw + 0.5;
  double g_a = -0.5;

  //  Reject events in prohibited regions.
  if (T > Enu) return 0.;

  //  Compute differential cross-section
  XC = pow((g_v + g_a), 2) + pow((g_v - g_a), 2) * pow((1. - T / Enu), 2) +
       (pow(g_a, 2) - pow(g_v, 2)) * (massElectron * T / pow(Enu, 2));

  XC *= Sigma0;

  //  Add term due to neutrino magnetic moment.
  static const double alphainv = 1. / CLHEP::fine_structure_const;
  if (T > 0.) XC += (M_PI / pow(alphainv, 2) * pow(vMu, 2) / pow(massElectron, 2)) * (1. - T / Enu) / T;

  //  Convert to detector units and return.
  XC *= XCunits;

  return XC;
}

void SNgen::SetXSecMax(int ntry) {
  double xMax = XMaxDefault;

  for (int i = 0; i < ntry + 1; i++) {
    double Enu = GetRandomNumber(0., 10.);
    double T = GetRandomNumber(0., Enu);
    double xsec = GetXSec(Enu, T);
    if (xsec > xMax) xMax = xsec;
  }

  XSecMax = xMax;
  SetNormFlag(true);
}

double SNgen::GetRandomNumber(double rmin, double rmax) {
  double rnd = G4UniformRand();  // random number from 0 to 1.
  double value = rmin + (rmax - rmin) * rnd;
  return value;
}

double IBDTGraph2TF1(Double_t *x, Double_t *) { return graphIBD->Eval(x[0]); }
double ESTGraph2TF1(Double_t *x, Double_t *) { return graphES->Eval(x[0]); }
double CCTGraph2TF1(Double_t *x, Double_t *) { return graphCC->Eval(x[0]); }
double ICCTGraph2TF1(Double_t *x, Double_t *) { return graphICC->Eval(x[0]); }
double NCTGraph2TF1(Double_t *x, Double_t *) { return graphNC->Eval(x[0]); }
double NCNUTGraph2TF1(Double_t *x, Double_t *) { return graphNCRate->Eval(x[0]); }
double INCTGraph2TF1(Double_t *x, Double_t *) { return graphINC->Eval(x[0]); }

// Reactions Generator, Modified by Will

double SNgen::GetIBDRandomEnergy(Double_t &dt) {
  // Define Variables
  Double_t ran_IBD = glum_eb->GetRandom();
  dt = ran_IBD;
  Double_t meran_IBD = gmene_eb->Eval(ran_IBD);
  Double_t alran_IBD = galpha_eb->Eval(ran_IBD);
  Double_t IBD_return;
  Double_t sigma_0;
  sigma_0 = 1.705 * TMath::Power(10, -44);
  Double_t gA;
  gA = -1.23;
  Double_t del_np;
  del_np = 1.29332;
  Double_t m_e;
  m_e = 0.511;
  Double_t m_n;
  m_n = 939.57;
  // Calculate Flux & XS
  TF1 *IBD_Flux = new TF1("IBD_Flux",
                          "(TMath::Power(x,[0]))/(TMath::Gamma([0]+1))*TMath::Power(([0]+1)/"
                          "[1],[0]+1)*exp(-(([0]+1)/[1])*x)",
                          1.806, 100);
  IBD_Flux->SetParameters(alran_IBD, meran_IBD);
  TF1 *IBD_XS = new TF1("IBD_XS",
                        "((([0]*[1]*[2]*((1+3*([3]*[3]))/4)*(TMath::Power(((x-[4])/"
                        "[5]),2))*sqrt(1-(([5]/(x-[4]))*([5]/(x-[4]))))*(1+((1.1*x/[6]))))))",
                        1.806, 100);
  IBD_XS->SetParameters(1, 1, sigma_0, gA, del_np, m_e, m_n);
  TF1 *IBD_Total = new TF1("IBD_Total", "IBD_Flux*IBD_XS", 1.806, 100);  // combo line
  IBD_return = IBD_Total->GetRandom();
  IBD_Flux->Delete();
  IBD_XS->Delete();
  IBD_Total->Delete();
  return IBD_return;
}
double SNgen::GetESRandomEnergy(Double_t &dt) {
  // Here, a random number is chosen to select e, eb or and then it is similar
  // to IBD above
  TF1 *fran = new TF1("fran", "abs(sin(x)/x)*sqrt(x)", 0, 1);
  Double_t ES_return;
  Double_t ran_ES;
  ran_ES = fran->GetRandom();
  if (ran_ES <= 0.167) {
    Double_t ran_ES_e = glum_e->GetRandom();
    dt = ran_ES_e;
    Double_t meran_ES_e = gmene_e->Eval(ran_ES_e);
    Double_t alran_ES_e = galpha_e->Eval(ran_ES_e);
    Double_t sigma_0_ESe;
    sigma_0_ESe = 1.705 * TMath::Power(10, -44);
    Double_t m_e_ESe;
    m_e_ESe = 0.511;
    Double_t cos_cab_ESe;
    cos_cab_ESe = 0.80795;
    Double_t sin_wei_ESe;
    sin_wei_ESe = 1.29332;
    TF1 *ES_e_Flux = new TF1("ES_e_Flux",
                             "(TMath::Power(x,[0]))/(TMath::Gamma([0]+1))*TMath::Power(([0]+1)/"
                             "[1],[0]+1)*exp(-(([0]+1)/[1])*x)",
                             0, 100);
    ES_e_Flux->SetParameters(alran_ES_e, meran_ES_e);
    TF1 *ES_e_XS = new TF1("ES_e_XS", "([0]/(8*[1]))*(x/[2])*(1+4*[3]+(16/3)*[3]*[3])", 0, 100);
    ES_e_XS->SetParameters(sigma_0_ESe, cos_cab_ESe, m_e_ESe, sin_wei_ESe);
    TF1 *ES_e_Total = new TF1("ES_e_Total", "ES_e_Flux*ES_e_XS", 0, 100);
    ES_return = ES_e_Total->GetRandom();
    ES_e_Flux->Delete();
    ES_e_XS->Delete();
    ES_e_Total->Delete();
  } else if (ran_ES <= 0.33) {
    Double_t ran_ES_eb = glum_eb->GetRandom();
    dt = ran_ES_eb;
    Double_t meran_ES_eb = gmene_eb->Eval(ran_ES_eb);
    Double_t alran_ES_eb = galpha_eb->Eval(ran_ES_eb);
    Double_t sigma_0_ESeb;
    sigma_0_ESeb = 1.705 * TMath::Power(10, -44);
    Double_t m_e_ESeb;
    m_e_ESeb = 0.511;
    Double_t cos_cab_ESeb;
    cos_cab_ESeb = 0.80795;
    Double_t sin_wei_ESeb;
    sin_wei_ESeb = 1.29332;
    TF1 *ES_eb_Flux = new TF1("ES_eb_Flux",
                              "(TMath::Power(x,[0]))/(TMath::Gamma([0]+1))*TMath::Power(([0]+1)/"
                              "[1],[0]+1)*exp(-(([0]+1)/[1])*x)",
                              0, 100);
    ES_eb_Flux->SetParameters(alran_ES_eb, meran_ES_eb);
    TF1 *ES_eb_XS = new TF1("ES_eb_XS", "([0]/(8*[1]))*(x/[2])*(1+4*[3]+(16/3)*[3]*[3])", 0, 100);
    ES_eb_XS->SetParameters(sigma_0_ESeb, cos_cab_ESeb, m_e_ESeb, sin_wei_ESeb);
    TF1 *ES_eb_Total = new TF1("ES_eb_Total", "ES_eb_Flux*ES_eb_XS", 0, 100);  // combo line
    ES_return = ES_eb_Total->GetRandom();
    ES_eb_Flux->Delete();
    ES_eb_XS->Delete();
    ES_eb_Total->Delete();
  } else {
    Double_t ran_ES_x = glum_x->GetRandom();
    dt = ran_ES_x;
    Double_t meran_ES_x = gmene_x->Eval(ran_ES_x);
    Double_t alran_ES_x = galpha_x->Eval(ran_ES_x);
    Double_t sigma_0_ESx;
    sigma_0_ESx = 1.705 * TMath::Power(10, -44);
    Double_t m_e_ESx;
    m_e_ESx = 0.511;
    Double_t cos_cab_ESx;
    cos_cab_ESx = 0.80795;
    Double_t sin_wei_ESx;
    sin_wei_ESx = 1.29332;
    TF1 *ES_x_Flux = new TF1("ES_x_Flux",
                             "(TMath::Power(x,[0]))/(TMath::Gamma([0]+1))*TMath::Power(([0]+1)/"
                             "[1],[0]+1)*exp(-(([0]+1)/[1])*x)",
                             0, 100);
    ES_x_Flux->SetParameters(alran_ES_x, meran_ES_x);
    TF1 *ES_x_XS = new TF1("ES_x_XS", "([0]/(8*[1]))*(x/[2])*(1+4*[3]+(16/3)*[3]*[3])", 0, 100);
    ES_x_XS->SetParameters(sigma_0_ESx, cos_cab_ESx, m_e_ESx, sin_wei_ESx);
    TF1 *ES_x_Total = new TF1("ES_x_Total", "ES_x_Flux*ES_x_XS", 0, 100);
    ES_return = ES_x_Total->GetRandom();
    ES_x_Flux->Delete();
    ES_x_XS->Delete();
    ES_x_Total->Delete();
  }
  return ES_return;
}
double SNgen::GetCCRandomEnergy(Double_t &dt) {
  Double_t CC_return;
  Double_t ran_CC = glum_e->GetRandom();
  dt = ran_CC;
  Double_t meran_CC = gmene_e->Eval(ran_CC);
  Double_t alran_CC = galpha_e->Eval(ran_CC);
  TF1 *CC_Flux = new TF1("CC_Flux",
                         "(TMath::Power(x,[0]))/(TMath::Gamma([0]+1))*TMath::Power(([0]+1)/"
                         "[1],[0]+1)*exp(-(([0]+1)/[1])*x)",
                         0, 100);
  CC_Flux->SetParameters(alran_CC, meran_CC);
  TF1 *CC_XS = new TF1("CC_XS", "4.7e-40*(TMath::Power(x,0.25)-TMath::Power(15,0.25))**6", 0, 100);
  TF1 *CC_Total = new TF1("CC_Total", "CC_Flux*CC_XS", 0, 100);
  CC_return = CC_Total->GetRandom();
  CC_Flux->Delete();
  CC_XS->Delete();
  CC_Total->Delete();
  //            printf("**************************** CC Event
  //            ****************************** \n");
  return CC_return;
}
double SNgen::GetICCRandomEnergy(Double_t &dt) {
  Double_t ICC_return;
  Double_t ran_ICC = glum_eb->GetRandom();
  dt = ran_ICC;
  Double_t meran_ICC = gmene_eb->Eval(ran_ICC);
  Double_t alran_ICC = galpha_eb->Eval(ran_ICC);
  TF1 *ICC_Flux = new TF1("ICC_Flux",
                          "(TMath::Power(x,[0]))/(TMath::Gamma([0]+1))*TMath::Power(([0]+1)/"
                          "[1],[0]+1)*exp(-(([0]+1)/[1])*x)",
                          0, 100);
  ICC_Flux->SetParameters(alran_ICC, meran_ICC);
  TF1 *ICC_XS = new TF1("ICC_XS", "7.2e-41*(TMath::Power(x,0.25)-TMath::Power(9.5,0.25))**6", 0, 100);
  TF1 *ICC_Total = new TF1("ICC_Total", "ICC_Flux*ICC_XS", 0, 100);
  ICC_return = ICC_Total->GetRandom();
  ICC_Flux->Delete();
  ICC_XS->Delete();
  ICC_Total->Delete();
  //            printf("******************************* ICC Event
  //            ****************************** \n");
  return ICC_return;
}
double SNgen::GetNCRandomEnergy(Double_t &dt) {  //
  Double_t NC_return;
  Double_t ran_NC = glum_e->GetRandom();
  dt = ran_NC;
  Double_t meran_NC = gmene_e->Eval(ran_NC);
  Double_t alran_NC = galpha_e->Eval(ran_NC);
  TF1 *NC_Flux = new TF1("NC_Flux",
                         "(TMath::Power(x,[0]))/(TMath::Gamma([0]+1))*TMath::Power(([0]+1)/"
                         "[1],[0]+1)*exp(-(([0]+1)/[1])*x)",
                         0, 100);
  NC_Flux->SetParameters(alran_NC, meran_NC);
  TF1 *NC_XS = new TF1("NC_XS", "9.7e-41*(TMath::Power(x,0.25)-TMath::Power(12.75,0.25))**6", 0, 100);
  TF1 *NC_Total = new TF1("NC_Total", "NC_Flux*NC_XS", 0, 100);
  NC_return = NC_Total->GetRandom();
  NC_Flux->Delete();
  NC_XS->Delete();
  NC_Total->Delete();
  printf(
      "******************************* NC Event "
      "********************************** \n");
  return NC_return;
}
double SNgen::GetNCRandomNuEnergy() {
  Double_t NCR_return;
  Double_t ran_NCR = glum_e->GetRandom();
  // dt = ran_NCR; // NC Nu is different to the others
  Double_t meran_NCR = gmene_e->Eval(ran_NCR);
  Double_t alran_NCR = galpha_e->Eval(ran_NCR);
  TF1 *NCR_Flux = new TF1("NCR_Flux",
                          "(TMath::Power(x,[0]))/(TMath::Gamma([0]+1))*TMath::Power(([0]+1)/"
                          "[1],[0]+1)*exp(-(([0]+1)/[1])*x)",
                          0, 100);
  NCR_Flux->SetParameters(alran_NCR, meran_NCR);
  TF1 *NCR_XS = new TF1("NCR_XS", "9.7e-41*(TMath::Power(x,0.25)-TMath::Power(12.75,0.25))**6", 0, 100);
  TF1 *NCR_Total = new TF1("NCR_Total", "NCR_Flux*NCR_XS", 0, 100);
  NCR_return = NCR_Total->GetRandom();
  NCR_Flux->Delete();
  NCR_XS->Delete();
  NCR_Total->Delete();
  //            printf("******************************* NCR Event
  //            ********************************** \n");
  return NCR_return;
}
double SNgen::GetINCRandomEnergy(Double_t &dt) {
  Double_t INC_return;
  Double_t ran_INC = glum_e->GetRandom();
  dt = ran_INC;
  Double_t meran_INC = gmene_e->Eval(ran_INC);
  Double_t alran_INC = galpha_e->Eval(ran_INC);
  TF1 *INC_Flux = new TF1("INC_Flux",
                          "(TMath::Power(x,[0]))/(TMath::Gamma([0]+1))*TMath::Power(([0]+1)/"
                          "[1],[0]+1)*exp(-(([0]+1)/[1])*x)",
                          0, 100);
  INC_Flux->SetParameters(alran_INC, meran_INC);
  TF1 *INC_XS = new TF1("INC_XS", "9.7e-41*(TMath::Power(x,0.25)-TMath::Power(12.75,0.25))**6", 0, 100);
  TF1 *INC_Total = new TF1("INC_Total", "INC_Flux*INC_XS", 0, 100);
  INC_return = INC_Total->GetRandom();
  INC_Flux->Delete();
  INC_XS->Delete();
  INC_Total->Delete();
  //            printf("******************************* INC Event
  //            ********************************** \n");
  return INC_return;
}

// End of section added by Will

void SNgen::LoadSpectra() {
  int model = GetModel();
  Double_t magsumTot, totIBD, totES, totCC, totICC, totNC, x, y;

  info << "\n===================== Supernova information ======================" << newline;

  // Load in the Livermore model
  if (model == 1) {
    info << "\nUsing the livermore model. Within the this model the "
            "following rates are present.\nThese rates are not used in the "
            "processing of these events, but may be set by the user\nmanualy "
         << newline;

    //////////////////////////// IBD
    /////////////////////////////////////////////////////////

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_ibd");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    graphIBD = new TGraph();
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      magsumTot += (spec_mag[istep]);
      graphIBD->SetPoint(istep, spec_E[istep], spec_mag[istep]);
    }
    info << "Total IBD integrate spectra is  " << magsumTot << newline;
    totIBD = magsumTot;

    //////////////////////////// CC
    /////////////////////////////////////////////////////////

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_nue_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    graphCC = new TGraph();
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      magsumTot += (spec_mag[istep]);
      graphCC->SetPoint(istep, spec_E[istep], spec_mag[istep]);
    }
    info << "Total CC integrate spectra is  " << magsumTot << newline;
    totCC = magsumTot;

    //////////////////////////// ICC
    /////////////////////////////////////////////////////////

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_nuebar_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    graphICC = new TGraph();
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      magsumTot += (spec_mag[istep]);
      graphICC->SetPoint(istep, spec_E[istep], spec_mag[istep]);
    }
    info << "Total ICC integrate spectra is  " << magsumTot << newline;
    totICC = magsumTot;

    //////////////////////////// NC
    /////////////////////////////////////////////////////////

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_nc_nue_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    graphNCRate = new TGraph();

    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      magsumTot += (spec_mag[istep]);
      graphNCRate->SetPoint(istep, spec_E[istep], spec_mag[istep]);
    }
    //            info << "Total NC integrate spectra is  " <<
    //            magsumTot<<newline;
    totNC = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_nc_numu_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphNCRate->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphNCRate->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totNC = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_nc_nutau_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphNCRate->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphNCRate->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totNC = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_nc_nuebar_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphNCRate->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphNCRate->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totNC = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_nc_numubar_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphNCRate->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphNCRate->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totNC = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_nc_nutaubar_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphNCRate->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphNCRate->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totNC = magsumTot;

    //////////////////////////// ES
    /////////////////////////////////////////////////////////

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_nue_e");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    graphES = new TGraph();
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      magsumTot += (spec_mag[istep]);
      graphES->SetPoint(istep, spec_E[istep], spec_mag[istep]);
      // info << "ES nue " << spec_E[istep]  << " " <<spec_mag[istep] <<
      // newline;
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totES = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_nuebar_e");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphES->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphES->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
      // info << "ES nuebar x:(" << spec_E[istep]  << ", " <<x << "), E:("<<
      // spec_mag[istep] << " + " << y <<" = " << spec_mag[istep]+y << ")"
      // <<newline;
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totES = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_numu_e");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphES->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphES->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);

      // info << "ES numu x:(" << spec_E[istep]  << ", " <<x << "), E:("<<
      // spec_mag[istep] << " + " << y <<" = " << spec_mag[istep]+y << ")"
      // <<newline;
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totES = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_numubar_e");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphES->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphES->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
      // info << "ES numubar x:(" << spec_E[istep]  << ", " <<x << "), E:("<<
      // spec_mag[istep] << " + " << y <<" = " << spec_mag[istep]+y << ")"
      // <<newline;
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totES = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_nutau_e");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphES->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphES->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
      // info << "ES nutau x:(" << spec_E[istep]  << ", " <<x << "), E:("<<
      // spec_mag[istep] << " + " << y <<" = " << spec_mag[istep]+y << ")"
      // <<newline;
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totES = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "livermore_nutaubar_e");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphES->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphES->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
      // info << "ES nutaubar x:(" << spec_E[istep]  << ", " <<x << "), E:("<<
      // spec_mag[istep] << " + " << y <<" = " << spec_mag[istep]+y << ")"
      // <<newline;
    }
    info << "Total ES integrate spectra is  " << magsumTot << newline;
    totES = magsumTot;

    //////////////////////////// TALLY
    /////////////////////////////////////////////////////////

    Double_t tot = totIBD + totES + totCC + totICC + totNC;
    info << "(ibd,es,cc,icc,nc): "
         << "(" << totIBD / tot << ", " << totES / tot << ", " << totCC / tot << ", " << totICC / tot << ", "
         << totNC / tot << ")" << newline << newline;
  }  // GVKM MODELgvkm
  else if (model == 2) {
    info << "\nUsing the gvkm model. Within the this model the following "
            "rates are present.\nThese rates are not used in the processing "
            "of these events, but may be set by the user\nmanualy  "
         << newline;

    //////////////////////////// IBD
    /////////////////////////////////////////////////////////

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_ibd");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    graphIBD = new TGraph();
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      magsumTot += (spec_mag[istep]);
      graphIBD->SetPoint(istep, spec_E[istep], spec_mag[istep]);
    }
    info << "Total IBD integrate spectra is  " << magsumTot << newline;
    totIBD = magsumTot;

    //////////////////////////// CC
    /////////////////////////////////////////////////////////

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_nue_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    graphCC = new TGraph();
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      magsumTot += (spec_mag[istep]);
      graphCC->SetPoint(istep, spec_E[istep], spec_mag[istep]);
    }
    info << "Total CC integrate spectra is  " << magsumTot << newline;
    totCC = magsumTot;

    //////////////////////////// ICC
    /////////////////////////////////////////////////////////

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_nuebar_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    graphICC = new TGraph();
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      magsumTot += (spec_mag[istep]);
      graphICC->SetPoint(istep, spec_E[istep], spec_mag[istep]);
    }
    info << "Total ICC integrate spectra is  " << magsumTot << newline;
    totICC = magsumTot;

    //////////////////////////// NC
    /////////////////////////////////////////////////////////

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_nc_nue_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    graphNCRate = new TGraph();

    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      magsumTot += (spec_mag[istep]);
      graphNCRate->SetPoint(istep, spec_E[istep], spec_mag[istep]);
    }
    //            info << "Total NC integrate spectra is  " <<
    //            magsumTot<<newline;
    totNC = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_nc_numu_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphNCRate->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphNCRate->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totNC = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_nc_nutau_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphNCRate->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphNCRate->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totNC = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_nc_nuebar_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphNCRate->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphNCRate->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totNC = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_nc_numubar_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphNCRate->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphNCRate->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totNC = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_nc_nutaubar_O16");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphNCRate->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphNCRate->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totNC = magsumTot;

    //////////////////////////// ES
    /////////////////////////////////////////////////////////

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_nue_e");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    graphES = new TGraph();
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      magsumTot += (spec_mag[istep]);
      graphES->SetPoint(istep, spec_E[istep], spec_mag[istep]);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totES = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_nuebar_e");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphES->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphES->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totES = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_numu_e");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphES->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphES->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totES = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_numubar_e");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphES->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphES->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totES = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_nutau_e");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphES->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphES->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    //            info << "Total ES integrate spectra is  " <<
    //            magsumTot<<newline;
    totES = magsumTot;

    _lspec = DB::Get()->GetLink("SN_SPECTRUM", "gvkm_nutaubar_e");
    // Flux function
    spec_E.clear();
    spec_mag.clear();
    spec_E = _lspec->GetDArray("spec_e");
    spec_mag = _lspec->GetDArray("spec_mag");
    magsumTot = 0.0;
    for (unsigned int istep = 0; istep < spec_E.size(); ++istep) {
      graphES->GetPoint(istep, x, y);
      magsumTot += (spec_mag[istep] + y);
      graphES->SetPoint(istep, spec_E[istep], spec_mag[istep] + y);
    }
    info << "Total ES integrate spectra is  " << magsumTot << newline;
    totES = magsumTot;

    //////////////////////////// TALLY
    /////////////////////////////////////////////////////////

    Double_t tot = totIBD + totES + totCC + totICC + totNC;
    info << "(ibd,es,cc,icc,nc): "
         << "(" << totIBD / tot << ", " << totES / tot << ", " << totCC / tot << ", " << totICC / tot << ", "
         << totNC / tot << ")" << newline << newline;
  }

  // Neutral current event get a special treatment.

  graphINC = new TGraph();
  graphINC->SetPoint(0, 0, 0.0);
  graphINC->SetPoint(1, 5.25, 0.0);
  graphINC->SetPoint(2, 5.30, 0.73);
  graphINC->SetPoint(3, 5.35, 0.0);
  graphINC->SetPoint(4, 6.28, 0);
  graphINC->SetPoint(5, 6.33, 0.84);
  graphINC->SetPoint(6, 6.38, 0.0);
  graphINC->SetPoint(7, 7.18, 0);
  graphINC->SetPoint(8, 7.23, 0.29);
  graphINC->SetPoint(9, 7.28, 0.0);
  graphINC->SetPoint(10, 7.51, 0);
  graphINC->SetPoint(11, 7.56, 0.05);
  graphINC->SetPoint(12, 7.61, 0.0);
  graphINC->SetPoint(13, 8.27, 0);
  graphINC->SetPoint(14, 8.32, 0.07);
  graphINC->SetPoint(15, 8.37, 0.0);
  graphINC->SetPoint(16, 8.52, 0);
  graphINC->SetPoint(17, 8.57, 0.05);
  graphINC->SetPoint(18, 8.62, 0.0);
  graphINC->SetPoint(19, 9.11, 0);
  graphINC->SetPoint(20, 9.16, 0.31);
  graphINC->SetPoint(21, 9.21, 0.0);
  graphINC->SetPoint(22, 9.78, 0);
  graphINC->SetPoint(23, 9.83, 0.14);
  graphINC->SetPoint(24, 9.88, 0.0);
  graphINC->SetPoint(25, 10.0, 0.0);

  graphNC = new TGraph();
  graphNC->SetPoint(0, 0, 0.0);
  graphNC->SetPoint(1, 5.20, 0.0);
  graphNC->SetPoint(2, 5.25, 0.28);
  graphNC->SetPoint(3, 5.30, 0.0);
  graphNC->SetPoint(4, 6.13, 0);
  graphNC->SetPoint(5, 6.18, 0.21);
  graphNC->SetPoint(6, 6.23, 0.0);
  graphNC->SetPoint(7, 6.71, 0);
  graphNC->SetPoint(8, 6.76, 0.14);
  graphNC->SetPoint(9, 6.81, 0.0);
  graphNC->SetPoint(10, 7.23, 0);
  graphNC->SetPoint(11, 7.28, 0.02);
  graphNC->SetPoint(12, 7.31, 0.0);
  graphNC->SetPoint(13, 10.0, 0.0);

  funcIBD = new TF1("funcIBD", IBDTGraph2TF1, 2, 100, 0);
  funcES = new TF1("funcES", ESTGraph2TF1, 2, 100, 0);
  funcCC = new TF1("funcCC", CCTGraph2TF1, 2, 100, 0);
  funcICC = new TF1("funcICC", ICCTGraph2TF1, 2, 100, 0);
  funcNC = new TF1("funcNC", NCTGraph2TF1, 0, 10, 0);
  funcNCNU = new TF1("funcNCNU", NCNUTGraph2TF1, 2, 100, 0);
  funcINC = new TF1("funcINC", INCTGraph2TF1, 0, 10, 0);
}

}  // namespace RAT
