es
''
::

    /generator/vtx/set  dir_x dir_y dir_z

Elastic-scattering events caused by the interaction of a neutrino with an
electron.  The event is initialized with the product of the reaction, an
electron.  The initial direction of the neutrino is along the (dir_x, dir_y,
dir_z) vector.  The neutrino energy is drawn from the spectrum given in the
[wiki:RATDB_IBD IBD table] by default, however several solar and stopped-pion
spectra can be used instead. The electron direction distribution is weighted
by the differential cross section of the interaction.

Note that the flux for elastic scattering is taken from the [wiki:RATDB_IBD IBD
table] values by default; that is, it's the same neutrinos that cause both types of
events.

To take the flux for elastic scattering from one of the solar spectra in 
`ratdb/SOLAR.db`, specify the process and neutrino flavor:

::

    /generator/vtx/set  dir_x dir_y dir_z SOLAR:process:nu_flavor

To take the flux for elastic scattering from one of the stopped-pion 
spectra in `ratdb/STPI.db`, specify the timing profile and neutrino flavor:

::

    /generator/vtx/set  dir_x dir_y dir_z STPI:timing_profile:nu_flavor


There are two parameters that control the elastic-scattering cross-section that
can be controlled by macro commands:

::

    /generator/es/wma  sin_squared_theta


This command sets the value of sine-squared of the weak mixing angle; the
default is 0.2277.

::

    /generator/es/vmu  neutrino_magnetic_moment


This command sets the value of the neutrino magnetic moment (units are Bohr
magnetons); the default is 0.
