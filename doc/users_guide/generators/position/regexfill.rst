regexfill
'''''''''
::

    /generator/pos/set volume_pattern
    
Events uniformly fill all physical volumes with names matching the `POSIX
regular expression (regex)
<https://en.wikipedia.org/wiki/Regular_expression#POSIX_basic_and_extended>`_
given as ``volume_pattern``. 
In general volume names correspond to the index of ``GEO`` table entries, but
complex geometry factories may generate other volumes as sub components, many
volumes for arrays, or both.

For a concrete example, this can be used to generate events in the wall (glass)
of all PMTs built with a ``pmtarray`` type geometry factory. 
If the index of the ``GEO`` table for the ``pmtarray`` was ``inner_pmts`` the
PMTID number of the physical PMT would be appended to the volume name as it is
created, so an appropriate regex would be ``inner_pmts[0-9]+`` ::

    /generator/pos/set inner_pmts[0-9]+

Note that the volume name is considered a match if the regex matches any part
of the volume name, e.g. the regex ``mts1`` would match the volume name
``inner_pmts100``. 
This can be avoided by using start ``^`` and end ``$`` of line characters when
specifying a unique ``^volume$`` by name.
