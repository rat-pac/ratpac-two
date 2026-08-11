AmBe source
'''''''''''
::

    /generator/add ambe POSITION:TIME

or

::

    /generator/add ambe POSITION

Creates a new AmBe generator using the specified position, and time generators.
If the variant without a TIME parameter is used, it implies the "poisson"
time generator.

This generator models the products of the alpha-n reaction of the AmBe source:
:math:`^{9}\mathrm{Be} + \alpha \rightarrow\ ^{12}\mathrm{C} + n`. It generates
the emitted neutron and the 4.43 MeV gamma produced in the subsequent
de-excitation of :math:`^{12}\mathrm{C}`. The alphas produced by the decay of
:math:`^{241}\mathrm{Am}` which induce the reaction are not simulated.

The generator is configured using the database entries in the
``AMBE_NSPECTRUM`` ratdb table at `ratdb/AMBE_NSPECTRUM.ratdb`. They are:

=========================  ===================
**Name**                   **Description**
=========================  ===================
``prob_gamma_emission``    The probability that a gamma is emitted with the
                           neutron. If no neutrons are simulated, the gamma is
                           always emitted.
``n_neutron``              The number of neutrons emitted per decay. Defaults
                           to 1 if not provided.
``energy_spectrum``        The energies in MeV used for the neutron kinetic
                           energy spectrum in ascending order.
``energy_prob``            The relative probability density at each energy in
                           the ``energy_spectrum``.
=========================  ===================

The generator produces the neutron and gamma at the specified position,
isotropically, at t=0 for each event. The neutron kinetic energy is drawn
from the specified ``energy_spectrum`` and ``energy_prob``. The spectrum is
automatically normalized by the generator. When emitted, the gamma energy is
set to the energy of the first excited state of :math:`^{12}\mathrm{C}`, 4.43
MeV. Simulations of just the neutron can be performed by setting
``prob_gamma_emission = 0``::

    /rat/db/set AMBE_NSPECTRUM prob_gamma_emission 0

while simulations of just the gamma can be performed by setting
``n_neutron = 0``::

    /rat/db/set AMBE_NSPECTRUM n_neutron 0
