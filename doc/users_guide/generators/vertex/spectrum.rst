spectrum
''''''''

Generates particles with isotropic momentum and kinetic energy drawn from a
user-defined spectrum stored in a SPECTRUM table in RATDB.  The spectrum is
linearly interpolated between points, which do not have to be uniformly spaced.

Example::

    /generator/vtx/set e- flat

Produces electron events drawn from the spectrum stored in the SPECTRUM[flat]
table::

    {
    name: "SPECTRUM",
    index: "flat",
    valid_begin: [0, 0],
    valid_end: [0, 0],
    
    // default spectrum is flat
    spec_e:     [ 1.00, 1.50, 2.00, 2.50, 3.00, 3.50, 4.00, 4.50, 5.00], // (MeV) 
    // (Note that first point is minimum of spectrum, last is maximum)
    spec_mag:   [ 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00], // don't worry about normalisation 
    }
