Gsim Generators
---------------
Gsim creates the initial particles simulated in the event using ''generators'',
which are enabled in the macro file.  A generator decides how often a
particular kind of event occurs, where it happens, and what kind of particles
and energies the event starts with.  Multiple generators can be used at once,
and different events will be interleaved according to their rates, or even pile
up if they occur in coincidence.

Generators are activated in the macro file using the command::

    /generator/add gen_name  generator_options

The first parameter, ''gen_name'', identifies the kind of event generator being
added.  For example, the ''combo'' generators allows you to piece together
separate vertex, position, and time generators.  The second parameter,
''generator_options'' is a string which is passed to the generator itself to
configure it.  For example::

    /generator/add combo gun:point:poisson

adds a new combo generator to the simulation which will be comprised of a
particle gun with events filling a detector volume and poisson-distributed
random times.

Once a generator has been added, you can configure the vertex, position, and
time components.  For example, we can generate isotropic positrons in the
center with 1 MeV of kinetic energy at a mean rate of 1 per second with the
commands::

    /generator/vtx/set 0 0 0  1.0
    /generator/pos/set 0 0 0
    /generator/rate/set 1.0

.. include:: generators/toplevel.rst
.. include:: generators/vertex.rst
.. include:: generators/position.rst
.. include:: generators/time.rst
