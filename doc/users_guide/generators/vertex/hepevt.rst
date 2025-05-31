hepevt
''''''

::

   /generator/vtx/set file_name

This generator takes a HEPEvt file as an input. The generator will read
the file and generate events based on the information in the file.
HEPEvt is a legacy file format typically used with FORTRAN-based
generators, but is also supported by some modern generators such as
MARLEY. The file should contain the event information in a specific
format, which includes particle information such as PDG codes, momenta,
and vertex positions.

Note that since this is only a vertex generator, it does not utilize the
position and timing information provided in the HEPEvt file. Combination
with a position and timing generator is still required.

Detailed documentation of the format is documented in detail by MARLEY
`here <https://www.marleygen.org/interpret_output.html#hepevt>`__.
