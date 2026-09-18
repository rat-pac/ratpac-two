pileup
''''''
::

    /generator/add pileup VERTEX:POSITION:TIME

or

::

    /generator/add pileup VERTEX:POSITION

Creates a new pileup generator using the vertex and position generators
described below. If the variant without a TIME parameter is used, it implies
the "poisson" time generator. The TIME generator controls the spacing
between separate pileup *events*; within each event, the generator produces
multiple primary vertices from the same VERTEX/POSITION configuration,
with a count and per-vertex timing set by the commands below.

::

    /generator/pileup/multiplicity fixed:N
    /generator/pileup/multiplicity uniform:min:max
    /generator/pileup/multiplicity poisson:mean

Sets the distribution for the number of vertices generated per event.
``fixed`` always generates exactly N vertices, ``uniform`` draws an integer
uniformly between min and max (inclusive), and ``poisson`` draws from a
zero-truncated Poisson distribution with the given mean (a sample of 0 is
rejected and redrawn, since every pileup event must contain at least one
vertex). Defaults to ``fixed:1``. N and min must be at least 1, and the
poisson mean must be greater than 0.

::

    /generator/pileup/timing fixed:dt
    /generator/pileup/timing uniform:min:max
    /generator/pileup/timing poisson:mean

Sets the distribution, in ns, for vertex times relative to the event start
time (TIME). ``fixed`` places vertices dt apart, starting at TIME.
``uniform`` draws each vertex's offset from TIME independently and
uniformly between min and max, then time-orders the results.
``poisson`` draws each vertex's offset from TIME independently from an
exponential distribution with the given mean, then time-orders the results.
Defaults to ``fixed:0``.

Example
--------

::

    /generator/pileup/multiplicity poisson:5
    /generator/pileup/timing uniform:0:1000

A constant-rate radioactive source observed over a fixed time window: the
number of decays in the window is Poisson-distributed, and conditioned on
that count, each decay's time within the window is independent and
uniformly distributed.
