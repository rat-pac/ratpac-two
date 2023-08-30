multipoint
''''''''''
::

    /generator/pos/set number_of_locations inner_radius outer_radius

Generates events at different locations in the detector between two radii.  For
a given value of number_of_locations, the points are unique and fixed for all
runs on all platforms.  The generator will cycle between the different points
as the event number increments, so the number of events you generate in each
job should be a multiple of number_of_locations.  This generator is typically
used to benchmark reconstruction, as you can fit events at each generated
location to compute a bias and resolution.
