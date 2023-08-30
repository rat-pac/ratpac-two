decaychain
''''''''''
::

    /generator/add decaychain ISOTOPE:POSITION:TIME

or

::

    /generator/add decaychain ISOTOPE:POSITION

Creates a new decaychain generator using the position, and time generators
described below.  If the variant without a TIME parameter is used, it implies
the "poisson" time generator.

The ISOTOPE parameter can be any chain or element found in the file
data/beta_decay.dat.  The alpha, beta, and gamma particles emitted by the
radioactive decay chain will be included in the event, with times and kinetic
energies randomly generated according to the physics of the decay.  In the
current implementation, the times are set such the final decay occurs at t=0,
so that earlier decays are at negative times.
