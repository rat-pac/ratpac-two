.. _physics:

Physics Processes
-----------------
The standard RAT simulation includes many standard GEANT4 physics processes.

Physics Lists
`````````````

Add documentation for the Physics List!

Cerenkov Radiation
``````````````````
The Cerenkov class implemented in ratpac-two simulations is based on the original GEANT4 11.1.2 class, with a few key updates and added features.
The source file can be found within src/physics/src/ThinnableG4Cerenkov.cc, which allows an effective thinning factor to be applied to the number of produced photons,
and the PMT efficiency is inversely scaled to account for this change (as long as the thinning factor does not increase the QE above 1.0).
The ThinnableG4Cerenkov class calls our own custom Cerenkov class, called nRangeG4Cerenkov, located in the same sub-directory. 
nRangeG4Cerenkov allows for the correct simulation of materials with non-monotonic refractive indices, which has been observed in noble liquids.
GEANT4 cannot handle these materials, which leads the original class to under-predict the photon yield.

There are a few commands users can use to 'tune' their simulation to match physical expectations. These are described below.

| Command | Function | Default Value |
| :------------------------------------------- | :---------------------------------------------------------------------------- | :--- |
| /rat/physics/SetCerenkovMaxNumPhotonsPerStep | Controls the maximum number of photons produced within a step (integer).      |   4  |
| /rat/physics/SetCerenkovMaxBetaChangePerStep | Controls the maximum change in the phase velocity within a step (percentage). | 10.0 |

-------------------

Dicebox
```````

Add documentation for dicebox.


