Californium source
''''''''''''''''''
::

    /generator/add cf 252:POSITION:TIME

or

::

    /generator/add cf 252:POSITION

Creates a new Cf252 generator using the position, and time generators described
below.  If the variant without a TIME parameter is used, it implies the
"poisson" time generator.

The syntax of the command may lead you to think that other isotopes of Cf252
(e.g., Cf255) are supported.  One day that may happen, but right now only the
value 252 can occur in the command, otherwise you'll get an error message.

This generator models the products of the spontaneous fission of Cf252:
neutrons and prompt photons.  It does *not* model the radioactive decay of
Cf252; neither does the ''decaychain'' generator above (though that could be
added by revising the data/beta_decay.dat file).  Note: Geant4 models the
fission of nuclei due to de-excitation from radioactive decays, and has its own
implementation of radioactive decay chains, but it does not include a model for
the spontaneous fission of nuclei; the only way to include that is by writing a
separate event generator.
