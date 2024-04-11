/**
 * @brief  Implementations for the ANNRIGd_156GdContinuumModelV2 class.
 * @author Sebastian Lorenz
 * @date   2017-11-06
 */

//==============================================================================
// INCLUDES

// ANNRIGdGammaSpecModel includes
#include "RAT/ANNRIGd_156GdContinuumModelV2.hh"

#include "RAT/ANNRIGd_Random.hh"
// ROOT includes
#include "TAxis.h"
#include "TFile.h"
#include "TH2D.h"
#include "TMath.h"
#include "TString.h"

// extern std::ofstream outf;
// extern int NumGamma;
// extern double GammaEnergies[15];

namespace Rnd = ANNRIGdGammaSpecModel::Random;
namespace Aux = ANNRIGdGammaSpecModel::Auxiliary;
using Aux::ParticleEnergies;
using Aux::ParticleEnergy;
using std::cerr;
using std::cout;
using std::endl;

//==============================================================================
// STATIC INITAILIZATIONS

int ANNRIGdGammaSpecModel::ANNRIGd_156GdContinuumModelV2::sInstanceCounter = 1;

//==============================================================================
// CLASS METHOD IMPLEMENTATIONS

namespace ANNRIGdGammaSpecModel {

//______________________________________________________________________________
/**
 * @brief   Constructor with parameter.
 * @details Starts initialization of the model.
 * 	        Sets '156GdContinuumModelV2' as model name.
 * @param   inDataFileName  Name of input data file name. Must not be empty.
 * @pre     Given name of input data file is not empty.
 */
ANNRIGd_156GdContinuumModelV2::ANNRIGd_156GdContinuumModelV2(const std::string& inDataFileName)
    : ANNRIGd_Model("156GdContinuumV2", ANNRIGd_ModelType::Mdl156GdContinuum),
      eMax_(8.536),  // neutron separation energy of 156Gd in MeV
      dE_(0.0),
      instID_(sInstanceCounter),
      lut_(0) {
  ++sInstanceCounter;
  Initialize(inDataFileName);
}

//______________________________________________________________________________
/**
 * @brief   Copy-constructor.
 * @details A deep copy is made by cloning all referred to by raw pointers.
 *          For ROOT-objects, new names based on the instance ID of this
 *          object are assigned.
 * @param   other  ANNRIGd_156GdContinuumModelV2 instance that shall be copied.
 */
ANNRIGd_156GdContinuumModelV2::ANNRIGd_156GdContinuumModelV2(const ANNRIGd_156GdContinuumModelV2& other)
    : ANNRIGd_Model("156GdContinuumV2", ANNRIGd_ModelType::Mdl156GdContinuum),
      eMax_(8.536),  // neutron separation energy of 156Gd in MeV
      dE_(other.dE_),
      instID_(sInstanceCounter),
      lut_(0) {
  ++sInstanceCounter;

  // clone histogram and set name
  if (other.lut_) lut_ = static_cast<TH2D*>(other.lut_->Clone(TString::Format("contModelLut_%i", instID_)));
}

//______________________________________________________________________________
//! @brief Destructor.
ANNRIGd_156GdContinuumModelV2::~ANNRIGd_156GdContinuumModelV2() {
  delete lut_;
  lut_ = 0;
}

//______________________________________________________________________________
/**
 * @brief   Copy-assignment operator.
 * @details Uses copy-and-swap. The old member objects referred to by raw pointers
 *          are deleted.
 * @param   other  ANNRIGd_156GdContinuumModelV2 instance to assign by copy.
 * @return  Reference to this instance.
 */
ANNRIGd_156GdContinuumModelV2& ANNRIGd_156GdContinuumModelV2::operator=(const ANNRIGd_156GdContinuumModelV2& other) {
  TH2D* lut = static_cast<TH2D*>(other.lut_->Clone("Gd156contModelLut_0"));

  delete lut_;
  lut_ = lut;
  lut = 0;
  lut_->SetName(TString::Format("Gd156contModelLut_%i", instID_));

  dE_ = other.dE_;

  return *this;
}

//------------------------------------------------------------------------------
// PRIVATE METHODS

//______________________________________________________________________________
/**
 * @brief   Generates gamma-rays from the continuum component of 156Gd* after
 *          the thermal 155Gd(n,g) reaction.
 * @details While the residual energy (being the neutron binding energy at the
 *          beginning) is larger than one energy step dE in the look-up table,
 *          new gamma-ray energies are looked up based on a random number
 *          and the current excitation energy (= residual energy). The look-up
 *          is done based on a random number from ]0,1[.
 *          The returned reaction products are all gamma-rays. No IC electrons
 *          are considered. All reaction products have random directions.
 * @return  Vector containing the ReactionProduct objects for gamma-rays with
 *          random directions an energies in MeV corresponding to the
 *          continuum part of the thermal 155Gd(n,g) reaction.
 */
ReactionProductVector ANNRIGd_156GdContinuumModelV2::DoGenerate() const {
  ParticleEnergies energies;

  double eRes = eMax_;  // residual energy in MeV

  while (eRes > 0.2) {
    // determine gamma-ray energy; compute residual energy
    double eGamma = GetGammaEnergy(eRes);
    eRes -= eGamma;
    if (eRes < 0.0) {
      eGamma += eRes;
      eRes = 0.0;
    }

    // store gamma energy
    energies.push_back(ParticleEnergy(22, eGamma));
  }

  if (eRes > 0.0) {
    energies.push_back(ParticleEnergy(22, eRes));
  }

  // save gamma multiplicity
  // NumGamma = energies.size();
  // for (int i=0; i<NumGamma; i++) GammaEnergies[i] = energies[i].second;

  ReactionProductVector products;
  Aux::FillRndmDirProducts(products, energies);
  return products;
}

//______________________________________________________________________________
/**
 * @brief   Returns a random gamma ray energy for a given residual excitation
 *          energy.
 * @details A random number is generated. Based on this number and the given
 *          residual excitation energy, the corresponding gamma ray energy is
 *          looked up from the table. This value is corrected by a random, linear
 *          shift towards the gamma ray energy value in the next random number
 *          cell to further randomize the returned energy value.
 * @param   eRes  Residual excitation energy in MeV.
 * @return  Random gamma ray energy in MeV constrained by the given residual
 *          excitation energy.
 */
double ANNRIGd_156GdContinuumModelV2::GetGammaEnergy(double eRes) const {
  double rndm = Rnd::Uniform();
  const int bin = lut_->FindFixBin(eRes, rndm);
  int binx = 0;
  int biny = 0;
  int binz = 0;
  lut_->GetBinXYZ(bin, binx, biny, binz);
  const double e1 = lut_->GetBinContent(bin);
  const double e2 = biny <= lut_->GetNbinsY() ? lut_->GetBinContent(binx, biny + 1) : e1;
  const double dE = e1 - e2;

  double rndm2 = Rnd::Uniform();
  double eGamma = e1 - rndm2 * dE;
  eGamma = eGamma < 0.0 ? 0.0 : eGamma;

  return eGamma;
}

//______________________________________________________________________________
/**
 * @brief   Initializes the model.
 * @details Reads in the look-up table from the ROOT-file of given name.
 *          The application aborts if
 *          - the given input data file name is empty,
 *          - the input data file could not be opened.
 * @param   inDataFileName  Name of input data file. Must not be empty.
 * @pre     Given name of input data file is not empty.
 */
void ANNRIGd_156GdContinuumModelV2::Initialize(const std::string& inDataFileName) {
  cout << "ANNRIGd_156GdContinuumModelV2 : Initializing model..." << endl;

  if (not inDataFileName.empty()) {
    TFile* inFile = TFile::Open(inDataFileName.c_str(), "READ");
    if (inFile and not inFile->IsZombie()) {
      // get look-up table

      inFile->GetObject("contTbl", lut_);
      lut_->SetDirectory(0);
      lut_->SetName(TString::Format("Gd156contModelLut_%i", instID_));

      // determine dE
      const int nx = lut_->GetNbinsX();
      const double xmin = lut_->GetXaxis()->GetXmin();
      const double xmax = lut_->GetXaxis()->GetXmax();
      dE_ = nx > 0 ? (xmax - xmin) / nx : 0.0;

      delete inFile;
    } else {
      cerr << "ANNRIGd_156GdContinuumModelV2 : ERROR! Could not open "
              "input data file <"
           << inDataFileName << "> - ABORTING!" << endl;
      abort();
    }
  } else {
    cerr << "ANNRIGd_156GdContinuumModelV2 : ERROR! Given input data file "
            "name is empty - ABORTING!"
         << endl;
    abort();
  }

  cout << "ANNRIGd_156GdContinuumModelV2 : Done!" << endl;
}

} /* namespace ANNRIGdGammaSpecModel */
