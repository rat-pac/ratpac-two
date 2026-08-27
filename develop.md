# Direct-light-only photon tracking

Development notes for the `/tracking/directLightOnly` feature added to this
ratpac-two checkout.

## Goal

Simulate only *direct* Cherenkov and scintillation light: photons that travel in
a straight line from their emission point to the surface where they are
absorbed/detected, with no scattering, wavelength shifting, reemission or
reflection along the way.

## Bugs found in the original proof-of-concept

The starting point was this snippet in `GLG4SteppingAction::UserSteppingAction`:

```cpp
if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
      const G4VProcess *creator = track->GetCreatorProcess();
      if (creator && creator->GetProcessName() != "Cerenkov"&&creator && creator->GetProcessName() != "scitillation") {
          track->SetTrackStatus(fStopAndKill);
          return;
      }
      G4ThreeVector preStepDirection = aStep->GetPreStepPoint()->GetMomentumDirection();
      G4ThreeVector postStepDirection = aStep->GetPostStepPoint()->GetMomentumDirection();
      if (preStepDirection.dot(postStepDirection) < 0.96) {
        track->SetTrackStatus(fStopAndKill);
      }
}
```

1. **`"Cerenkov"` never matches.** RAT uses `RAT::ThinnableG4Cerenkov`, which
   inherits the default name of `G4CerenkovProcess`, i.e. **`"G4CerenkovProcess"`**.
   `Gsim.cc` compares with `.find("Cerenkov")` for exactly this reason.
2. **`"scitillation"` is a typo** and the real name is **`"Scintillation"`**
   (`GLG4Scint.cc:70`). Combined with (1), every clause of the `if` is true for
   every photon, so the snippet kills 100% of the optical photons.
3. **The pre/post step direction test kills refraction.** Fresnel transmission
   into acrylic/glass/a PMT window changes the direction legitimately, and such
   a photon is still direct light. Conversely a grazing total internal
   reflection deflects by less than the 16 degrees implied by `0.96` and
   survives. The test has both false positives and false negatives.
4. **Consecutive-step comparison lets deflections accumulate.** Ten 15 degree
   kinks each pass `dot < 0.96` individually.

Minor issues: `creator &&` is tested twice; a null creator process (generator
primaries) silently survives; and the early `return` skips the
`num_zero_steps_in_a_row` reset, leaking one track's zero-step count into the
next track.

## Implementation

Process-based rather than angle-based.

### `src/core/src/GLG4SteppingAction.cc`

New static configuration and `GLG4SteppingAction::ApplyDirectLightFilter()`,
called from the top of `UserSteppingAction` for optical photons when
`fDirectLightOnly` is set. It returns `true` when it killed the photon, in
which case the caller resets `num_zero_steps_in_a_row` before returning so the
shared counter does not carry over into the next track.

The filter has three parts:

* **Creator process.** Checked once, on step 1. The name is taken from
  `G4Track::GetCreatorProcess()`, overridden by `RAT::TrackInfo::GetCreatorProcess()`
  when set, matching what `Gsim` does when it classifies photons. It is matched
  as a *substring* against `fDirectLightProcesses`, so `"Cerenkov"` matches
  `"G4CerenkovProcess"` and `"Scintillation"` matches `"Scintillation"` while
  *not* matching `"Reemission"` / `"ReemissionFromCompN"`. An empty creator name
  means a primary photon from a generator and is always kept.
* **Scattering processes.** Killed on `OpRayleigh`, `Rayleigh`, `OpMieHG`,
  `OpWLS` and `OpWLS2`.
* **Boundaries.** Identified by comparing the post-step process pointer with the
  `G4OpBoundaryProcess` instance attached to the optical photon (looked up once
  per thread and cached), then dispatched on `GetStatus()`:
  * kept: `Undefined`, `Transmission`, `FresnelRefraction`, `NotAtBoundary`,
    `SameMaterial`, `StepTooSmall`, `NoRINDEX`, `Absorption`, `Detection`,
    `CoatedDielectricRefraction`, `CoatedDielectricFrustratedTransmission`;
  * killed: every reflection flavour, plus `Dichroic`, whose outcome is
    ambiguous and is treated conservatively as indirect.

An optional angular cut is measured against `GetVertexMomentumDirection()` (the
emission direction) rather than the previous step, so small kinks cannot
accumulate unnoticed. It is disabled by default.

### `src/cmd/src/TrackingMessenger.cc`, `src/cmd/include/RAT/TrackingMessenger.hh`

Three new macro commands, all no-ops unless the first is turned on:

| command | type | default |
| --- | --- | --- |
| `/tracking/directLightOnly` | bool | `false` |
| `/tracking/directLightProcesses` | string, space separated | `Cerenkov Scintillation` |
| `/tracking/directLightMinCosine` | double, `<= -1` disables | `-1.0` |

`GetCurrentValue` is implemented for all three, so `/control/getEnv`-style
queries and `/control/manual /tracking` work as for the pre-existing commands.

The messenger is constructed in `Rat.cc` before any macro is executed, so the
commands may be placed anywhere in the macro.

Example:

```
/run/initialize
/tracking/directLightOnly true
```

## Validation

Built and run inside `ratpac2.sif` (Geant4 11.4). 2.5 MeV electrons at the
origin of the `Validation/Valid.geo` geometry, PE counted per creator process
from the `outroot` output.

| run | events | PE by creator process |
| --- | --- | --- |
| water, feature off | 20 | 344 `G4CerenkovProcess` |
| water, `directLightOnly true` | 20 | 332 `G4CerenkovProcess` |
| water, `directLightProcesses Reemission` | 20 | 0 (sanity check that the filter bites) |
| water, `directLightMinCosine 0.9999` | 20 | 8 `G4CerenkovProcess` |
| LABPPO, feature off | 5 | 1575 `Scintillation`, 1915 `ReemissionFromComp1`, 1348 `ReemissionFromComp0`, 70 `G4CerenkovProcess` |
| LABPPO, `directLightOnly true` | 5 | 1290 `Scintillation`, 47 `G4CerenkovProcess`, 0 reemission |

The LABPPO runs used the same geometry with
`/rat/db/set GEO[world] material "scintillator_lab_ppo0p6"` and the same for
`GEO[detector]`.

Full `make` and `make install` succeed; `clang-format` applied per `cformat.sh`.

## Known limitation

Photons detected inside `GLG4PMTOpticalModel` are recorded by that fast
simulation model, so the hit is booked before the stepping action sees the step.
Reflections *inside* the PMT optical model are therefore not filtered. Handling
those would require a change in `src/physics/src/GLG4PMTOpticalModel.cc`.

## Files touched

* `src/core/include/RAT/GLG4SteppingAction.hh`
* `src/core/src/GLG4SteppingAction.cc`
* `src/cmd/include/RAT/TrackingMessenger.hh`
* `src/cmd/src/TrackingMessenger.cc`
