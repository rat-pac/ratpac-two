reacibd
'''
::

    /generator/vtx/set dir_x dir_y dir_z

Inverse beta decay events caused by the interaction of a reactor neutrino with
a stationary proton.  The initial energy of the neutrino for each event is
selected from a probability density function dependent on the total neutrino
flux from a reactor and the inverse beta-decay cross-section.  The initial
direction of the neutrino is along the (dir_x, dir_y, dir_z) vector.  The
positron direction is currently randomized relative to the neutrino's incident
direction.

The relative isotopic abundances of U235, U238, Pu239, and Pu241 can be
controlled using macro commands:

::

    /generator/reacibd/U235 U235Amp  #Default is 0.496
    /generator/reacibd/U238 U238Amp  #Default is 0.087
    /generator/reacibd/Pu239 Pu239Amp  #Default is 0.391
    /generator/reacibd/Pu241 Pu241Amp  #Default is 0.066

The abundances should be provided for all four isotopes and should add to 1.
The addition of other isotopes and elements is currently not supported.
