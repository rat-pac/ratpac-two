#include <RAT/Config.hh>
#if TENSORFLOW_Enabled
#include <TVector.h>
#include <cppflow/cppflow.h>
#include <cppflow/model.h>

#include <RAT/DB.hh>
#include <RAT/DS/EV.hh>
#include <RAT/DS/FitResult.hh>
#include <RAT/DS/PMTInfo.hh>
#include <RAT/DS/Root.hh>
#include <RAT/DS/RunStore.hh>
#include <RAT/FitTensorProc.hh>
#include <RAT/Log.hh>
#include <RAT/Processor.hh>
#include <RAT/Rat.hh>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <numeric>

namespace RAT {

FitTensorProc::FitTensorProc() : Processor("fittensor") {}

void FitTensorProc::BeginOfRun(DS::Run *run) {
  DB *db = DB::Get();
  DBLinkPtr table = db->GetLink("Fitter", "FitTensor");
  std::string directory = table->GetS("directory");

  std::string path = "";
  for (auto dir : Rat::model_directories) {
    std::string fullpath = dir + directory;
    if (std::filesystem::exists(fullpath)) {
      path = fullpath;
      break;
    }
  }
  if (!std::filesystem::exists(path)) {
    Log::Die("Could not find FitTensor path: " + path);
  } else {
    directionModel = new cppflow::model(path + "/direction");
    positionModel = new cppflow::model(path + "/position");
  }
}

Processor::Result FitTensorProc::Event(DS::Root *ds, DS::EV *ev) {
  DS::FitResult *fit = new DS::FitResult("FitTensor");

  TVector3 fitPosition = PositionFit(ds, ev);
  TVector3 fitDirection = DirectionFit(ds, ev, fitPosition);

  fit->SetPosition(fitPosition);
  fit->SetDirection(fitDirection);

  ev->AddFitResult(fit);

  return Processor::OK;
}

cppflow::tensor FitTensorProc::CreateProjection(DS::EV *ev, DS::PMTInfo *pmtinfo, TVector3 coordinates) {
  const int xdim = 26;
  const int ydim = 26;
  const double rail = 10.0;
  const double tmod = 10.0;
  std::vector<float> flatArray;
  std::vector<int64_t> shape = {1, xdim, ydim, 2};
  float red[xdim][ydim];
  float green[xdim][ydim];

  for (int pmtid : ev->GetAllPMTIDs()) {
    DS::PMT *pmt = ev->GetOrCreatePMT(pmtid);
    TVector3 pmtpos = pmtinfo->GetPosition(pmt->GetID());
    TVector3 pos = pmtpos - coordinates;
    double hittime = pmt->GetTime();
    double charge = pmt->GetCharge();
    // Image parameters
    int theta = int((pos.Z() / pos.Mag() + 1 - 1e-9) / 2 * xdim);
    int phi = int((atan2(pos.Y(), pos.X()) + M_PI - 1e-9) / 2 / M_PI * ydim);
    red[theta][phi] = hittime * tmod / 255;
    green[theta][phi] = charge / rail;
  }
  for (int i = 0; i < xdim; i++) {
    for (int j = 0; j < ydim; j++) {
      float red_value = red[i][j];
      red_value = std::isnan(red_value) ? 0 : red_value;
      // info << red_value << newline;
      flatArray.push_back(std::min(std::max(static_cast<float>(0.0), red_value), static_cast<float>(1.0)));
      float green_value = green[i][j];
      green_value = std::isnan(green_value) ? 0 : green_value;
      flatArray.push_back(std::min(std::max(static_cast<float>(0.0), green_value), static_cast<float>(1.0)));
    }
  }
  cppflow::tensor input(flatArray, shape);
  return input;
}

TVector3 FitTensorProc::PositionFit(DS::Root *ds, DS::EV *ev) {
  DS::Run *run = DS::RunStore::Get()->GetRun(ds);
  DS::PMTInfo *pmtinfo = run->GetPMTInfo();
  cppflow::tensor input = CreateProjection(ev, pmtinfo, TVector3(0.0, 0.0, 0.0));

  auto output =
      (*this->positionModel)({{"serving_default_conv2d_input:0", input}}, {"StatefulPartitionedCall:0"}).at(0);
  const std::vector<float> &positionFitResult = output.get_data<float>();
  TVector3 positionVector(positionFitResult.at(0), positionFitResult.at(1), positionFitResult.at(2));
  return positionVector;
}

TVector3 FitTensorProc::DirectionFit(DS::Root *ds, DS::EV *ev, TVector3 position) {
  DS::Run *run = DS::RunStore::Get()->GetRun(ds);
  DS::PMTInfo *pmtinfo = run->GetPMTInfo();
  cppflow::tensor input = CreateProjection(ev, pmtinfo, position);

  auto output =
      (*this->directionModel)({{"serving_default_conv2d_input:0", input}}, {"StatefulPartitionedCall:0"}).at(0);
  const std::vector<float> &directionfit = output.get_data<float>();
  TVector3 fitDirection(directionfit.at(0), directionfit.at(1), directionfit.at(2));
  double norm = fitDirection.Mag();
  fitDirection.SetX(fitDirection.X() / norm);
  fitDirection.SetY(fitDirection.Y() / norm);
  fitDirection.SetZ(fitDirection.Z() / norm);
  return fitDirection;
}

}  // namespace RAT
#endif
