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

-------------------

Dicebox
```````

Add documentation for dicebox.


