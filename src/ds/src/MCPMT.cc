#include <RAT/DS/MCPMT.hh>

namespace RAT {
namespace DS {

std::string MCPMT::GetCreatorProcess() const {
  std::string process = "";
  double time = 9999.;
  for (size_t iph = 0; iph < photon.size(); iph++) {
    if (photon[iph].GetFrontEndTime() < time) {
      time = photon[iph].GetFrontEndTime();
      process = photon[iph].GetCreatorProcess();
    }
  }
  return process;
}

Double_t MCPMT::GetCharge() const {
  Double_t charge = 0.0;
  for (unsigned int i = 0; i < photon.size(); i++) charge += photon[i].GetCharge();
  return charge;
}

}  // namespace DS
}  // namespace RAT
