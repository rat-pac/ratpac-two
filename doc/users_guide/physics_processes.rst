.. _physics:

Physics Processes
-----------------
The standard RAT simulation includes many standard GEANT4 physics processes.

Physics Lists
`````````````

Add more documentation for the Physics List!

Commands that related to the physics list are listed under ``/rat/physics``.
You can find out what they do in ``src/cmd/src/PhysicsListMessenger.cc``.
Descriptions to these commands will be added here soon. 


Remove a process from the physics list
''''''''''''''''''''''''''''''''''''''
If you want to remove a process from the current physics list (e.g. if you
would like to stop simulating muon decays), you can modify the physics list in
your main macro file (e.g. run.mac) as follows:

    /rat/physics/removeProcess mu+ Decay
    /rat/physics/removeProcess mu- Decay

Where the first argument is the G4 name of the particle, and the second is the
name of the process you would like to remove. 

Cerenkov Radiation
``````````````````
The Cerenkov class implemented in ratpac-two simulations is based on the original GEANT4 11.1.2 class, with a few key updates and added features.
The base source file can be found within src/physics/src/ as RAT::G4CerenkovProcess, and is implemented within ThinnableG4Cerenkov.cc, which allows 
an effective thinning factor to be applied to the number of produced photons. The PMT efficiency is inversely scaled to account for the thinning factor 
(as long as the thinning factor does not increase the QE above 1.0).
RAT::G4CerenkovProcess allows for the correct simulation of materials with non-monotonic refractive indices, which has been observed in noble liquids.
GEANT4 could not handle these materials previously, which resulted in the original class under-predicting the Cerenkov photon yield.

There are a few commands users can use to 'tune' their simulation to match physical expectations. These are described below.

| Command | Function | Default Value |
| :------------------------------------------- | :---------------------------------------------------------------------------- | :--- |
| /rat/physics/enableCerenkov                  | Controls whether we simulate Cerenkov photons (bool | true / false).          | true |
| /rat/physics/setCerenkovMaxNumPhotonsPerStep | Controls the maximum number of photons produced within a step (integer).      |   4  |
| /rat/physics/setCerenkovMaxBetaChangePerStep | Controls the maximum change in the phase velocity within a step (percentage). | 10.0 |

-------------------

Dicebox
```````

Add documentation for dicebox.


