#include <cmath>
#include <RAT/IntegratedQuenchingCalculator.hh>
#include <RAT/AppliedQuenchingModel.hh>

IntegratedQuenchingCalculator::IntegratedQuenchingCalculator(BirksLaw model,
                                                     Quadrature* quadrature):
                                         QuenchingCalculator(model),
                                            fQuadrature(quadrature){
    /* */
}

double IntegratedQuenchingCalculator::QuenchedEnergyDeposit(const G4Step& step,
                                                            const double kB){
    const G4ParticleDefinition* def = step.GetTrack()->GetParticleDefinition();
    const G4Material* mat = step.GetPostStepPoint()->GetMaterial();
    //Short circuit issues with 0 energy steps
    if (step.GetDeltaEnergy() == 0){
        return 0;
    }
    // one might think that the total energy loss is the difference in kinetic
    // energy, but alas, this is not guaranteed! we must also explicitly check
    // that we do not integrate over negative energies, for which the stopping
    // power is undefined.
    const double E_before = step.GetPreStepPoint()->GetKineticEnergy();
    const double E_after = fmax(E_before - step.GetTotalEnergyDeposit(), 0.0);
    AppliedQuenchingModel callable(this->model, def, mat, kB);
    const double rv = this->fQuadrature->Integrate(callable, E_after, E_before);
    return rv;
}
