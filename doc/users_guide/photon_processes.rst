.. _photon_processes:

Optical Photon Processes
------------------------

Cherenkov
`````````

To generate Cherenkov light, ratpac-two uses the ``Geant4`` G4Cerenkov class, with some small changes. The class in ratpac-two is called ``ThinnableG4Cerenkov`` as it primarily add the ability to 'thin' the number of photons that get propagated in ratpac-two. This option is provided in order to increase the speed of the simulation, and is discussed in more detail in :ref:`photon_thinning`. 

------------------------

Scintillation 
`````````````
('''DISCLAIMER''': While the scintillation code in RAT is based on GLG4Sim by
Glenn Horton-Smith, we have made several modifications to the code which change
its behavior.  Assume all bugs are ours!)

The scintillation simulation in RAT is handled differently than all other
physics processes.  In order to conserve energy on a step-by-step basis,
scintillation photons are computed not as a standard GEANT4 physics process,
but rather as a separate task after all other physics processes have run.  The
scintillation code can then look at the energy deposited during that completed
step and calculate the number of scintillation photons that would be generated.
A secondary task of the scintillation code is to handle reemission of photons
in volumes which contain wavelength-shifter.

Code Structure
''''''''''''''
When the [source:rat/src/core/Gsim.cc Gsim::Init()] method is called, all of
the GEANT4 user callbacks are established.  One of these callbacks is the for a
custom G4UserSteppingAction called [source:rat/src/core/GLG4SteppingAction
GLG4SteppingAction].  At the end of each step, this class performs several
tasks, among which is calling the static method
[source:rat/src/core/GLG4Scint.cc GLG4Scint::GenericPostPostStepDoIt()].
GLG4Scint::!GenericPostPostStepDoIt() returns at G4VParticleChange object which
contains the new secondary tracks (either scintillation photons or wavelength
shifted photons) to be registered with the GEANT4 Stepping Manager.

In order to handle particle-specific scintillation parameters, a list of
GLG4Scint objects are built by GLG4PhysicsList at startup, each responsible for
a different particle.  The static [source:rat/src/core/GLG4Scint.cc
GLG4Scint::GenericPostPostStepDoIt()] method picks one of these objects based
on the mass of the particle in the track.  This list of particles is current
limited to::

 * (default)
 * neutron
 * alpha
 * Ne20
 * Ar39
 * Ar40

If scintillation parameters are not specified for one of these particle types,
the GLG4Scint object will load the default parameters instead.  Once a suitable
GLG4Scint object has been identified for the track, the
GLG4Scint::!PostPostStepDoIt() method is called.   The rest of this page
describes what GLG4Scint::!PostPostStepDoIt() actually does.

Scintillator Definition
'''''''''''''''''''''''
The model parameters discussed below are defined as single-valued variables or arrays that are typically
a function of photon wavelength in nm or time in ns.  Most of the parameters can be defined to represent the
bulk response of a scintillator, but can also be defined for each component of the scintillator (identified
by a numerical index appended to the variable/array name).  Component information will be used only if the
number of components ''NUM_COMP'' is defined.  Otherwise, non-component varibales/arrays will be used for
the material as a whole.  Non-component information can be used even if ''NUM_COMP'' is defined.
If ''NUM_COMP'' is not defined or set to 0, no component information will be used.

Computing Number of Scintillation Photons
'''''''''''''''''''''''''''''''''''''''''
Normal particles (i.e. not optical photons) can deposit energy gradually in the
medium through ionization and other processes.  At the end of each track step,
GLG4Scint determines the total deposited energy, ''dE'', and the step length,
''dx''.  Then it applies Birks' Law to compute the deposited energy after
quenching:

.. math::

    dE_{\rm quench} = \frac{dE}{1 + B \times dE/dx}

where ''B'' is Birks' constant for your scintillator.  If ''B'' is set to zero,
then Birks' Law has no effect and the scintillator response is independent of
''dE/dx''. Birks' constant gets defined as one of the ''SCINTMOD'' parameters (key = 1), along with resolution
scale (key = 0) and the reference ''dE/dx'' (key = 2) discussed below.  These parameters can be defined per
particle type.

An additional particle-dependent quenching factor, ''P(E)'' can also be set
which depends on the kinetic energy of the particle at the end of the step.
This is useful if the scintillator quenching has been measured directly for a
range of energies.  This quenching factor array gets defined as ''QF''.
A single value can also be defined through macro command ''setQF''.

The deposited energy is converted to scintillation photons using the product of
the total light yield (''Y'') of the scintillator (which is in units of photons per
MeV), the deposited energy, Birks' Law scaling, the particle-dependent
quenching, and a "reference ''dE/dx''" for Birks' Law.
The light yield variable gets defined as ''LIGHT_YIELD''.
The reference ''dE/dx'' gets defined as noted above and is useful if you have measured the light yield of
the scintillator only with highly ionizing particles, like alphas, which already have a significant Birks'
Law component.  The reference dE/dx effectively removes the quenching already
in the light yield.

Finally, the mean number of photons can be scaled down by the "Photon Thinning"
factor (''T'') selected by the user.  Photon thinning is used to accelerate the
simulation by reducing the number of optical photons produced by a constant
factor, and then increasing the PMT photocathode efficiency by the same factor
such that the product of light yield and detection efficiency is held constant.

Put together, the mean number of scintillation photons produced in the step is

.. math::

    N = Y \times dE \times \frac{1 + B \times dE/dx_{\rm ref}}{1 + B \times dE/dx} \times P(E) \times T

Most of the factors in this equation are optional, and if not specified default
to 1 for ''P(E)'' and ''T'' and 0 for ''B'' and ''dE/dx_{ref}''.

The actual number of scintillation photons produced in the step is drawn from a
Poisson distribution with mean N.

Light Yield
'''''''''''
If LIGHT_YIELD is not defined, then it will be extracted as the integral of the SCINTILLATION spectrum (or spectra) and
a warning will be printed.  A LIGHT_YIELD can be defined for each scintillator component.

Scintillation Spectrum
''''''''''''''''''''''
Once the number of scintillation photons has been specified, the photon energy is drawn from the supplied SCINTILLATION
spectrum, which can be defined per particle type.  A non-component SCINTILLATION spectrum and a component SCINTILLATION#
spectrum are not allowed to exist simultaneously in the OPTICS definition of a material.
If no SCINTILLATION specturm exists, then the material will not scintillate,
though it could still re-emit.  This can be used to simulate a wavelength shifter (see below).
The direction of each photon is randomly drawn from an isotropic distribution, and the polarization
vector is randomly selected, but constrained to be orthogonal to the direction
vector.  The position of the photon is drawn from a uniform distribution along
the line connecting the start and end points of the step.

Time Structure
''''''''''''''
The scintillation process can have a time structure associated with it.  The
start time of a scintillation photon is the time the particle passed through
the origin point of the photon, plus a delay drawn from the user-specified
distribution SCINTWAVEFORM, which can also have a separately defined SCINT_RISE_TIME.
A SCINTWAVEFORM can be defined for each scintillator component and per particle type.
There are three possible options for this delay distribution:

1. A sampled time distribution, in the form of a list of (time, intensity)
   pairs.
2. A sum of decaying exponential distributions, each with an associated
   branching fraction and time constant.
3. A sum of two decaying exponential distributions, whose time constants are a
   function of particle energy.

------------------------

Wavelength Shifting
```````````````````
There are a few ways of doing bulk wavelength shifting in RAT. The default
behavior is for GLG4Scint to handle opticalphotons as well as charged
particles. Alternatively, you can also let GLG4Scint handle the primary
scintillation, then use Geant4's G4OpWLS process or the custom BNLOpWLSModel
to do the reemission.

GLG4Scint Model
'''''''''''''''
The previous sections only apply to particles other than optical photons.
Optical photons are ignored by GLG4Scint, *except* when the photon is absorbed
inside the medium, but not at a geometry boundary.  If the photon is absorbed
in the bulk, then it is possible that it was absorbed by wavelength-shifter
present in the scintillator.

The spectrum of the outgoing photons is drawn from a SCINTILLATION_WLS distribution, which is defined separately from the
primary scintillation distribution and can also be defined per particle type.  A non-component SCINTILLATION_WLS spectrum
and a component SCINTILLATION_WLS spectrum are not allowed to exist simultaneously in the OPTICS definition of a material.
If no SCINTILLATION_WLS specturm exists, then the material will not re-emit.

The decision whether to reemit a photon is made by looking at the REEMISSION_PROB array, which gives the probability of
photon re-emission as a function of wavelength of the absorbed photon, for each absorbing scintillator component.

Multiple photon re-emission is possible and off by default.  (NOTE: TPB is one example of a material that shifts extreme
UV light to visible light, where it is energetically possible for more than one photon to be produced.)
To activate, set the REEMISSION_MULT variable to be > 0.  The wavelength of re-emitted photons (and number if
REEMISSION_MULT > 0) is determined by randomly sampling the availble portion of the re-emission spectrum until the sum of
energies of re-emitted photons is greater than the absorbed photon.

Re-emitted photons are delayed from their absorption time according to a unique time distribution, REEMITWAVEFORM.  This
distribution can be defined for each absorbing scintillator component.  The emission time of multiple re-emitted photons
is random and uncorrelated.

G4OpWLS Model
'''''''''''''
Choose this model in the macro with::

    /PhysicsList/setOpWLS g4

before calling initialize. See the Geant4 documentation for more details on the
required material properties.

BNLOpWLS Model
''''''''''''''
Choose this model in the macro with::

    /PhysicsList/setOpWLS bnl

This was written by L. Bignell at BNL to better model measurements of
scintillator cocktails with secondary fluors. The reemission spectrum (and
probability) is sampled depending on the photon wavelength, based on measured
data. The file to read this data from is in RATDB, in
`BNL_WLS_MODEL[].data_path`, which defaults to `data/ExEmMatrix.root`. The
reemission time can be set to either a delta function or an exponential
distribution, but currently is hard-coded to use an exponential. The latter is
set through the property in the OPTICS table `WLSTIMECONSTANT`.

This model also requires OPTICS properties `QUANTUMYIELD` (vector, decides how
many secondary photons to generate) and `WLSCOMPONENT` (vector, WLS wavelength
intensity) for WLS materials.

This WLS model has been validated by Chao Zhang of BNL. See these slides for
details:
:download:`bnl_wls_validation.pdf <bnl_wls_validation.pdf>`.

Quenching Models
''''''''''''''''

------------------------

.. _photon_thinning:

Photon Thinning
```````````````

Describe photon thinning here.

