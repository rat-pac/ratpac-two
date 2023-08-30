ibd
'''
::

    /generator/vtx/set  dir_x dir_y dir_z

Inverse beta decay events caused by the interaction of a neutrino with a
stationary proton.  The event is initialized with the products of the reaction,
a positron and a free neutron.  The initial direction of the neutrino is along
the (dir_x, dir_y, dir_z) vector.  The neutrino energy is drawn from the
spectrum given in the [wiki:RATDB_IBD IBD table], and the positron direction
distribution is weighted by the differential cross section of the interaction.
