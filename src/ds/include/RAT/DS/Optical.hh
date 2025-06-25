
/**
 * @class DS::Optical
 * Data Structure: Monte Carlo information
 *
 * @author Sean Hughes <sgshugh0@liverpool.ac.uk>
 *
 * Contains the information relating to printing out optical properties as meta data
 *
 */

#ifndef __RAT_DS_Optical__
#define __RAT_DS_Optical__

#include <TAxis.h>
#include <TGraph.h>
#include <TObject.h>
#include <TTimeStamp.h>

#include <RAT/DB.hh>

namespace RAT {
namespace DS {

class Optical : public TObject {
 public:
  Optical() : TObject() {}
  virtual ~Optical() {}

  /**
   * function that produces optical properties as TGraphs from existing ratdb tables.
   * Takes from all optics ratdb tables
   */
  std::pair<std::vector<TString>, std::vector<TGraph*>> GetOpticalProperty() {
    GetOPTICS();
    return std::make_pair(opticalProperties_names, opticalProperties);
  }

  void GetOPTICS() {
    DBLinkGroup mats = DB::Get()->GetLinkGroup("OPTICS");
    std::vector<std::string> opticalNames = {"ABSLENGTH",         "RINDEX",         "REFLECTIVITY", "SCINTILLATION",
                                             "SCINTILLATION_WLS", "REEMISSION_PROB"};

    std::string value1 = "_value1";
    std::string value2 = "_value2";

    // Load everything in OPTICS
    for (std::string opticalName : opticalNames) {
      for (DBLinkGroup::iterator iv = mats.begin(); iv != mats.end(); iv++) {
        std::string name = iv->first;
        DBLinkPtr table = iv->second;
        try {
          std::vector<double> abslength_wavelength = table->GetDArray((opticalName + value1));
          std::vector<double> abslength_value = table->GetDArray((opticalName + value2));
        } catch (DBNotFoundError& e) {
          std::cout << "Optics failed to get " << opticalName << " on: " << name << std::endl;
          continue;
        }

        std::cout << "Loading " << opticalName << " optics: " << name << std::endl;

        std::vector<double> abslength_wavelength = table->GetDArray((opticalName + value1));
        std::vector<double> abslength_value = table->GetDArray((opticalName + value2));

        TGraph* new_gr = new TGraph(abslength_value.size());

        for (size_t i = 0; i < abslength_wavelength.size(); ++i) {
          new_gr->SetPoint(i, abslength_wavelength.at(i), abslength_value.at(i));
        }
        new_gr->GetXaxis()->SetTitle("Wavelength [nm]");
        new_gr->GetYaxis()->SetTitle(opticalName.c_str());
        opticalProperties.push_back(new_gr);
        opticalProperties_names.push_back((name + "_" + opticalName));
      }
    }
  }

 private:
  std::string name;
  std::vector<TString> opticalProperties_names;
  std::vector<TGraph*> opticalProperties;
};
// ClassDef(Optical, 2);

}  // namespace DS
}  // namespace RAT

#endif
