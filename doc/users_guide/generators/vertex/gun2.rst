gun2
''''
::

    /generator/vtx/set  pname  px  py  pz angle E1 E2 [polx poly polz multiplicity]

Modification of gun, the single particle gun.  Creates a particle identified by
pname (as above) with initial momentum (px, py, pz) given in arbitrary units
for pointing. 
The angle parameter sets the opening angle of a 'cone of fire'  such that angle
= 90 fires particles evenly into the hemisphere along the [px,py,pz] direction.
Setting angle to 0 gives the same behavior as gun.  

E1 and E2 determine the range of particle kinetic energies in MeV.  Setting E1
and E2 the same results in the same behavior as gun.  If E2 != E1 the particle
energy is randomly drawn from a flat distribution between E1 and E2. 

The optional polarization vector of the particle is given by (polx, poly,
polz).

The optional multiplicity is the number of primaries shot at once.

If px=py=pz=0, then the gun generates particles with isotropic initial
directions.  Similarly, if polx=poly=polz=0, or the polarization vector is left
out, the particles will be randomly polarized. If the multiplicity is not 
specified, one primary will be shot.
