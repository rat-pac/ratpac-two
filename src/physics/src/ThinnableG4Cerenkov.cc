#include <RAT/ThinnableG4Cerenkov.hh>
#include <RAT/Log.hh>
#include <vector>

namespace RAT {

ThinnableG4Cerenkov::ThinnableG4Cerenkov() : should_thin(false),
                                             thinning_factor(1.0){
    this->heprandom = CLHEP::HepRandom();
}

void ThinnableG4Cerenkov::SetThinningFactor(double thinning){
    if ((thinning > 0) && (thinning <= 1.0)){
        this->should_thin = true;
        this->thinning_factor = thinning;
    }
    else{
        Log::Die(dformat("Cannot thin photons with acceptance %1.f%", thinning));
    }
}

double ThinnableG4Cerenkov::GetThinningFactor(){
    double rv = this->thinning_factor;
    return rv;
}

G4VParticleChange* ThinnableG4Cerenkov::PostStepDoIt(const G4Track& aTrack, const G4Step& aStep){
    // let G4 predict how many photons should be produced...
    G4VParticleChange* rv = G4Cerenkov::PostStepDoIt(aTrack, aStep);

    if (!should_thin){
        return rv;
    }

    // but only choose a fraction thereof to actually propagate
    G4int n_pred = rv->GetNumberOfSecondaries();
    // TODO a fixed length here could speed things up...
    // but is not (?) predictable, since we don't know outcome of following
    // per-photon RNG decisions
//  G4int n_prod = static_cast<G4int>(std::round(n_pred));
    std::vector<G4Track*> secondaries;
    for (G4int i = 0 ; i < n_pred ; i++){
        double random = heprandom.flat();
        if (random < this->GetThinningFactor()){
            // G4VParticleChange::SetNumberOfSecondaries will free all pending
            // secondaries, so we must explicitly copy each surviving track
            // into a separate data structure, which is a two-step process
            G4Track* existing = rv->GetSecondary(i);
            // first, copy data structure
            G4Track* secondary = new G4Track(*existing);
            // second, explicitly label tracking
            secondary->SetParentID(aTrack.GetTrackID());
            secondaries.push_back(secondary);
        }
    }

    // clear the list of secondaries, and repopulate with chosen subset
    rv->SetNumberOfSecondaries(secondaries.size());
    for (size_t i = 0 ; i < secondaries.size() ; i++){
        G4Track* secondary = secondaries[i];
        rv->AddSecondary(secondary);
    }

    return rv;
}

} // namespace RAT
