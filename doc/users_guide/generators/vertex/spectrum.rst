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

Example::

    /generator/vtx/set e- MICHEL

Produces Michel electron events drawn from the spectrum stored in the SPECTRUM[MICHEL] table. The spectrum is derived by integrating over cos(theta) using equation 56.3 in https://pdg.lbl.gov/2020/reviews/rpp2020-rev-muon-decay-params.pdf to get 3x**2 - 2x**3 where x = E_e / E_max and E_max = 52.8, and then normalizing to 1::

    {
    name: "SPECTRUM",
    index: "MICHEL",
    valid_begin: [0, 0],
    valid_end: [0, 0],

    // (Note that the energy spectrum has a maximum of 52.8 MeV)
    spec_e:     [ 0.00, 0.1, ..., 52.7, 52.8],
    // (Note that first point is minimum of spectrum, last is maximum)
    spec_mag:   [ 0.00000000,0.00000041, ..., 0.03780678, 0.03780718],
    }
