// This file needs a complete rewrite, it's a mess with incosistent style and
// tabs all over the place.
#include <G4LogicalBorderSurface.hh>
#include <G4PVPlacement.hh>
#include <G4SDManager.hh>
#include <RAT/DB.hh>
#include <RAT/DetectorConstruction.hh>
#include <RAT/Log.hh>
#include <RAT/Materials.hh>
#include <RAT/PMTConstruction.hh>
#include <RAT/PMTFactoryBase.hh>
#include <algorithm>
#include <vector>

#include "G4FastSimulationManager.hh"
#include "G4PhysicsOrderedFreeVector.hh"
#include "G4RandomDirection.hh"
#include "G4VFastSimulationModel.hh"
#include "RAT/GLG4PMTOpticalModel.hh"
#include "iostream"

namespace RAT {

DS::PMTInfo PMTFactoryBase::pmtinfo;

G4VPhysicalVolume *PMTFactoryBase::ConstructPMTs(DBLinkPtr table,
        const std::vector<G4ThreeVector> &pmt_pos, const std::vector<G4ThreeVector> &pmt_dir, const std::vector<int> &pmt_type,
        const std::vector<double> &pmt_effi_corr, const std::vector<double> &individual_noise_rate, const std::vector<double> &individual_afterpulse_fraction) {
    std::string volume_name = table->GetS("index");
    std::string mother_name = table->GetS("mother");
    std::string pmt_model = table->GetS("pmt_model");

    DBLinkPtr lpmt = DB::Get()->GetLink("PMT", pmt_model);

    // Find mother volume
    G4LogicalVolume *mother = FindMother(mother_name);
    if (mother == 0) {
        Log::Die("PMTParser: Unable to find mother volume " + mother_name + " for " + volume_name);
    }
    G4VPhysicalVolume *phys_mother = FindPhysMother(mother_name);
    if (phys_mother == 0) {
        Log::Die("PMTParser: PMT mother physical volume " + mother_name + " not found");
    }

    PMTConstruction *construction = PMTConstruction::NewConstruction(lpmt, mother);
    G4LogicalVolume *log_pmt = construction->BuildVolume(volume_name);

    /* BFields not in the database anywhere and should be looked at */
    // FIXME take a look at what's going on with the Bfield stuff - no docs on
    // this to be found

    // preparing to calculate magnetic efficiency corrections for all PMTs, if
    // requested
    int BFieldOn = 0;
    try {
        BFieldOn = DB::Get()->GetLink("BField")->GetI("b_field_on");
    }
    catch (DBNotFoundError &e) {
    }

    std::string BFieldTableName = "";
    std::string BEffiTableName = "";
    std::string dynorfilename = "";
    std::string BEffiModel = "multiplicative";
    DBLinkPtr BEffiTable;
    std::vector<G4PhysicsOrderedFreeVector> Bepsix, Bepsiy;
    std::vector<G4ThreeVector> Bpos, Bf;
    std::vector<G4ThreeVector> Dpos, Dorie;  // dynode position&orient

    // Force B efficiency<= 1 by default, whatever the input from BEffiTable.
    // If we start believing that B may actually help the PMT response, change
    // default to false
    bool CorrBEpsiInput = true;
    int nocorr = 0;
    try {
        nocorr = DB::Get()->GetLink("BField")->GetI("no_b_efficiency_table_correction");
    }
    catch (DBNotFoundError &e) {
    }
    if (nocorr) {
        CorrBEpsiInput = false;
    }
    else {
        info << "Forcing B efficiency<= 1" << newline;
    }
    bool HaveDynoData = false;

    if (BFieldOn) {
        try {
            BFieldTableName = DB::Get()->GetLink("BField")->GetS("b_field_file");
        }
        catch (DBNotFoundError &e) {
        }

        try {
            BEffiTableName = DB::Get()->GetLink("BField")->GetS("b_efficiency_table");
        }
        catch (DBNotFoundError &e) {
        }

        try {
            dynorfilename = DB::Get()->GetLink("dynorfile")->GetS("dynorfilename");
        } catch (DBNotFoundError &e) {
        }

        // check if we can calculate B field effect
        if (BFieldTableName == "" || BEffiTableName == "") {
            warn << "B field is on, but either B data or B PMT efficiency correction missing." << newline
                   << "Turning B field off." << newline;
            BFieldOn = 0;
            BEffiTable = NULL;
        }
        else {
            std::string ExpSubdir = DB::Get()->GetLink("DETECTOR")->GetS("experiment");
            std::string BFieldTableName1 = static_cast<std::string>(getenv("RATSHARE")) + "/ratdb/" + ExpSubdir + "/" + BFieldTableName;  // add the experiment subdir
            std::ifstream Bdata(BFieldTableName1.data());
            if (!Bdata.is_open()) {
                BFieldTableName = static_cast<std::string>(getenv("RATSHARE")) + "/ratdb/" + BFieldTableName;
                info << "file " << BFieldTableName1 << " not found, trying " << BFieldTableName << newline;
                Bdata.close();
                Bdata.open(BFieldTableName.data());
                if (!Bdata.is_open()) {
                    BFieldOn = false;
                    BEffiTable = NULL;
                    info << "also file " << BFieldTableName << " not found, magnetic efficiency correction turned off" << newline;
                }
                if (!Bdata.good()) {
                    Bdata.clear();  // for backwards compatibility: g++ 3.4 requires to
                }
                // manually reset the error state flags on opening a
                // new file with the same stream
            }

            std::string header;
            getline(Bdata, header);
            double xr, yr, zr, bxr, byr, bzr;
            info << "about to load B field from file " << BFieldTableName << newline;
            G4ThreeVector posi, field;
            while (!Bdata.rdstate()) {
                Bdata >> xr >> yr >> zr >> bxr >> byr >> bzr;
                posi.set(xr, yr, zr);
                field.set(bxr, byr, bzr);
                Bpos.push_back(posi);
                Bf.push_back(field);
            }
            BEffiTable = DB::Get()->GetLink(BEffiTableName);

            std::vector<double> bpmt;
            std::vector<double> epsix, epsiy;
            bpmt = BEffiTable->GetDArray("b");
            epsix = BEffiTable->GetDArray("deltax");
            epsiy = BEffiTable->GetDArray("deltay");
            G4PhysicsOrderedFreeVector *QBepsix;
            G4PhysicsOrderedFreeVector *QBepsiy;
            QBepsix = new G4PhysicsOrderedFreeVector();
            QBepsiy = new G4PhysicsOrderedFreeVector();
            for (int i = 0; i < int(bpmt.size()); i++) {
                QBepsix->InsertValues(bpmt[i], epsix[i]);
                QBepsiy->InsertValues(bpmt[i], epsiy[i]);
            }
            Bepsix.push_back(*QBepsix);
            Bepsiy.push_back(*QBepsiy);

            // no. of datasheets transcribed in the db table of the same PMT model
            int nsheets = 0;
            try {
                nsheets = BEffiTable->GetI("nsheets");
            }
            catch (DBNotFoundError &e) {
            }
            if (nsheets > 1) {
                char c[12];
                std::string name;
                for (int is = 1; is < nsheets; is++) {
                    sprintf(c, "%i", is);
                    name = static_cast<std::string>("deltax") + c;
                    epsix = BEffiTable->GetDArray(name);
                    name = static_cast<std::string>("deltay") + c;
                    epsiy = BEffiTable->GetDArray(name);
                    delete QBepsix;
                    delete QBepsiy;
                    QBepsix = new G4PhysicsOrderedFreeVector();
                    QBepsiy = new G4PhysicsOrderedFreeVector();
                    for (int i = 0; i < int(bpmt.size()); i++) {
                        QBepsix->InsertValues(bpmt[i], epsix[i]);
                        QBepsiy->InsertValues(bpmt[i], epsiy[i]);
                    }
                    Bepsix.push_back(*QBepsix);
                    Bepsiy.push_back(*QBepsiy);
                }
            }

            // try to load PMT orientation table from file
            dynorfilename = static_cast<std::string>(getenv("RATSHARE")) + "/ratdb/" + ExpSubdir + "/" + dynorfilename;
            std::ifstream dynorfile(dynorfilename.data());
            if (!dynorfile.is_open()) {
                info << "Failed to open " << dynorfilename.data() << ", will assume random dynode orientations" << newline;
                dynorfile.close();
            }
            else {
                getline(dynorfile, header);
                double xd, yd, zd, dynox, dynoy, dynoz;
                G4ThreeVector dor;
                while (!dynorfile.rdstate()) {
                    dynorfile >> xd >> yd >> zd >> dynox >> dynoy >> dynoz;
                    posi.set(xd, yd, zd);
                    dor.set(dynox, dynoy, dynoz);
                    Dpos.push_back(posi);
                    Dorie.push_back(dor);
                }
            }
            Bdata.close();
            dynorfile.close();

            if (!Dorie.empty() && Dorie.size() == Dpos.size()) {
                HaveDynoData = true;
            }
            else {
                info << "No dynode orientation datafile or error in the data, randomizing dynode orientations\n";
            }
        }

        try {
            BEffiModel = DB::Get()->GetLink("BField")->GetS("b_efficiency_model");
        }
        catch (DBNotFoundError &e) {
        }
        info << "\nSelected " << BEffiModel.data() << " B Efficiency Model" << newline;
    }
    else {
        BEffiTable = NULL;
    }

    // This will contain individual efficiency corrections for the placed PMTs
    std::map<int, double> EfficiencyCorrection;

    // Place physical PMTs
    // idx - the element of the particular set of arrays we are reading
    // id - the nth pmt that PMTFactoryBase has built
    for (size_t i = 0, id = pmtinfo.GetPMTCount(); i < pmt_pos.size(); i++, id++) {
        std::string pmtname = volume_name + "_pmtenv_" + std::to_string(id);  // internally PMTs are represented
        // by the nth pmt built, not pmtid

        G4ThreeVector pmtpos = pmt_pos[i];
        G4ThreeVector pmtdir = pmt_dir[i];

        // Store individual efficiency
        EfficiencyCorrection[id] = pmt_effi_corr[i];

        // Write the real (perhaps calculated) PMT positions and directions.
        // This goes into the DS by way of Gsim
        // NOTE: Since the BField stuff isn't used currently, add the efficiency here.
        // If we revive the BField code, this needs to be moved after so that the efficiency is correct
        pmtinfo.AddPMT(TVector3(pmtpos.x(), pmtpos.y(), pmtpos.z()), TVector3(pmtdir.x(), pmtdir.y(), pmtdir.z()),
                pmt_type[i], pmt_model, pmt_effi_corr[i], individual_noise_rate[i], individual_afterpulse_fraction[i]);

        // if requested, generates the magnetic efficiency corrections as the PMTs
        // are created
        if (BFieldOn) {
            // finds the point of the B grid closest to the current PMT, and
            // attributes it that Bfield
            double MinDist = DBL_MAX;
            int jmin = -1;
            for (int j = 0; j < int(Bpos.size()); j++) {
                if (MinDist > (pmtpos - Bpos[j]).mag()) {
                    MinDist = (pmtpos - Bpos[j]).mag();
                    jmin = j;
                }
            }

            if (jmin < 0) {
                info << "can't find a point close to the " << id << "-th pmt; MinDist is " << MinDist << newline;
            }
            else {
                G4ThreeVector bfield = Bf[jmin].perpPart(pmtdir);
                G4ThreeVector dynorient;
                if (HaveDynoData) {
                    int minj = -1;
                    double MinDiff = DBL_MAX;
                    for (int j = 0; j < int(Dorie.size()); j++) {
                        if ((pmtpos.unit() - Dpos[j].unit()).mag() < MinDiff) {
                            MinDiff = (pmtpos.unit() - Dpos[j].unit()).mag();
                            minj = j;
                        }
                    }

                    if (minj < 0) {
                        info << "can't find the orientation of the " << id << "-th pmt's dynode; MinDiff is " << MinDiff << newline
                             << "Throwing a random dynode orientation" << newline;
                        dynorient = G4RandomDirection();
                        dynorient = dynorient.perpPart(pmtdir);
                    }
                    else {
                        dynorient = Dorie[minj];
                    }
                }
                else {
                    // random dynode orientation
                    dynorient = G4RandomDirection();
                    // dynode orthogonal to PMT axis
                    dynorient = dynorient.perpPart(pmtdir);
                }

                if (dynorient.mag() == 0) {
                    for (int di = 0; di < 100 && dynorient.mag() == 0; di++) {
                        dynorient = G4RandomDirection();
                        dynorient = dynorient.perpPart(pmtdir);
                    }
                    if (dynorient.mag() == 0) {
                        info << "Warning: tried 100 times to generate a random dynode orientation for "
                             << id << "-th PMT and failed. dynorient " << dynorient(0) << "," << dynorient(1) << ","
                             << dynorient(2) << newline;
                    }
                }
                dynorient = dynorient.unit();

                // build EfficiencyCorrection table. PMT x axis is dynorient
                bool isOutRange;  // just a flag, not really used in the G4 code
                int sheetno = 0;
                if (Bepsix.size() > 1) {
                    double chooser = G4UniformRand();
                    if (chooser < 1. / 3) {
                        sheetno = 0;
                    }
                    else if (chooser < 2. / 3) {
                        sheetno = 1;
                    }
                    else {
                        sheetno = 2;
                    }
                }

                double BeffiComp;
                if (BEffiModel == "multiplicative") {
                    BeffiComp = Bepsix[sheetno].GetValue(bfield * dynorient, isOutRange) *
                        Bepsiy[sheetno].GetValue(bfield * (pmtdir.cross(dynorient)).unit(), isOutRange);

                    if (CorrBEpsiInput && BeffiComp > 1) {
                        EfficiencyCorrection[id] *= 1.0;
                    }
                    else {
                        EfficiencyCorrection[id] *= BeffiComp;
                    }
                }
                else if (BEffiModel == "additive") {
                    BeffiComp = Bepsix[sheetno].GetValue(bfield * dynorient, isOutRange) +
                        Bepsiy[sheetno].GetValue(bfield * (pmtdir.cross(dynorient)).unit(), isOutRange) - 1.;
                    if (CorrBEpsiInput && BeffiComp > 1) {
                        EfficiencyCorrection[id] *= 1.0;
                    }
                    else {
                        EfficiencyCorrection[id] *= BeffiComp;
                    }
                }
                else {
                    info << "\nError: undefined B Efficiency Model" << newline;
                }
            }
        }

        // rotation required to point in direction of pmtdir
        double angle_y = (-1.0) * atan2(pmtdir.x(), pmtdir.z());
        double angle_x = atan2(pmtdir.y(), sqrt(pmtdir.x() * pmtdir.x() + pmtdir.z() * pmtdir.z()));

        G4RotationMatrix *pmtrot = new G4RotationMatrix();
        pmtrot->rotateY(angle_y);
        pmtrot->rotateX(angle_x);

        construction->PlacePMT(pmtrot, pmtpos, pmtname, log_pmt, phys_mother, false, id);

    }  // end loop over id

    // finally pass the efficiency table to GLG4PMTOpticalModel
    const G4String modname(volume_name + "_optical_model");
    // In case the main pmt volume doesn't correspond to the fastsim region
    G4LogicalVolume *fastsim_log_pmt = log_pmt;
    if (fastsim_log_pmt->GetFastSimulationManager() == NULL) {
        fastsim_log_pmt = log_pmt->GetDaughter(0)->GetLogicalVolume();  // Get the glass region
    }

    for (size_t i = 0; i < fastsim_log_pmt->GetFastSimulationManager()->GetFastSimulationModelList().size(); i++) {
        if (fastsim_log_pmt->GetFastSimulationManager()->GetFastSimulationModelList()[i]->GetName() == modname) {
            ((GLG4PMTOpticalModel *)fastsim_log_pmt->GetFastSimulationManager()->GetFastSimulationModelList()[i])
                ->SetEfficiencyCorrection(EfficiencyCorrection);
            break;
        }
    }
    return 0;  // There is no specific physical volume to return
}

}  // namespace RAT
