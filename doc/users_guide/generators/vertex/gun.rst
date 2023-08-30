gun
'''
::

    /generator/vtx/set  pname  px  py  pz  [ke]  [polx poly polz]

Single particle gun.  Creates a particle identified by pname with initial
momentum (px, py, pz) given in MeV/c.  The optional parameter ke sets the
kinetic energy of the particle in MeV and overrides the magnitude of the
momentum vector.  (If you use ke, you can treat px, py, and pz as just a
direction vector.)  The optional polarization vector of the particle is given
by (polx, poly, polz).

If px=py=pz=0, then the gun generates particles with isotropic initial
directions.  Similarly, if polx=poly=polz=0, or the polarization vector is left
out, the particles will be randomly polarized.

Valid particle names include::

         GenericIon,                He3,              alpha,         anti_kaon0
        anti_lambda,       anti_neutron,          anti_nu_e,         anti_nu_mu
        anti_omega-,        anti_proton,        anti_sigma+,        anti_sigma-
        anti_sigma0,           anti_xi-,           anti_xi0,    chargedgeantino
           deuteron,                 e+,                 e-,                eta
          eta_prime,              gamma,           geantino,              kaon+
              kaon-,              kaon0,             kaon0L,             kaon0S
             lambda,                mu+,                mu-,            neutron
               nu_e,              nu_mu,             omega-,      opticalphoton
                pi+,                pi-,                pi0,             proton
             sigma+,             sigma-,             sigma0,             triton
                xi-,                xi0,

(This list comes from the /particle/list command.)
