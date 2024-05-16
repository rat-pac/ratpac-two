#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>

#include <RAT/Log.hh>
#include <RAT/PMTCoverageFactory.hh>
#include <vector>

namespace RAT {

G4VPhysicalVolume *PMTCoverageFactory::Construct(DBLinkPtr table) {
  G4double coverage = 0.0;   // coverage is percent solid angle
  G4double pmtRadius = 0.0;  // pmt radius is distance pmt to center
  G4double pmtDiameter;      // pmt geometry diameter

  try {
    coverage = table->GetD("coverage");
    pmtRadius = table->GetD("rescale_radius");
  } catch (DBNotFoundError &e) {
    Log::Die("PMTCoverageFactory: coverage or rescale_radius variables unset.");
  }

  try {
    pmtDiameter = table->GetD("pmtdiameter");
  } catch (DBNotFoundError &e) {
    pmtDiameter = 201.6;
  }

  // Below is almost completely stolen from ReactorFsim since it was
  // a feature people wanted ported.  Logic by MW. Wrapper CDT.
  int Ncosbins;
  int Nphibins;
  int Npmt;

  G4double num_pmt = 16 * pmtRadius * pmtRadius * coverage / pmtDiameter / pmtDiameter;
  Ncosbins = int(sqrt(num_pmt / CLHEP::pi));
  Nphibins = int(sqrt(num_pmt * CLHEP::pi));
  Npmt = Ncosbins * Nphibins;

  info << "PMTCoverageFactory: Generated " << Npmt << "PMTs" << newline;

  std::vector<double> xpmt(Npmt);
  std::vector<double> ypmt(Npmt);
  std::vector<double> zpmt(Npmt);

  G4double dphi = CLHEP::twopi / Nphibins;
  G4double dz = 2.0 / Ncosbins;

  for (int i = 0; i < Ncosbins; i++) {
    G4double z = -1 + i * dz + dz / 2;
    for (int j = 0; j < Nphibins; j++) {
      G4double phi = j * dphi + dphi / 2;
      xpmt[i * Nphibins + j] = pmtRadius * sqrt(1 - z * z) * cos(phi);
      ypmt[i * Nphibins + j] = pmtRadius * sqrt(1 - z * z) * sin(phi);
      zpmt[i * Nphibins + j] = pmtRadius * z;
    }
  }

  std::vector<G4ThreeVector> pos(Npmt), dir(Npmt);
  std::vector<double> individual_noise_rates(Npmt, 0.0);
  std::vector<double> individual_afterpulse_fraction(Npmt, 0.0);
  std::vector<int> type(Npmt, 0);
  std::vector<int> channel_number(Npmt, -1);
  std::vector<double> effi_corr(Npmt, 1.0);
  for (int i = 0; i < Npmt; i++) {
    pos[i].set(xpmt[i], ypmt[i], zpmt[i]);
    dir[i].set(-xpmt[i], -ypmt[i], zpmt[i]);
  }

  return ConstructPMTs(table, pos, dir, type, channel_number, effi_corr, individual_noise_rates,
                       individual_afterpulse_fraction);
}
}  // namespace RAT
