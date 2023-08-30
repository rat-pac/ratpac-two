fillshell
'''''''''
::

    /generator/pos/set X Y Z Ri Ro volname

Events uniformly fill a shell centered at (X, Y, Z) (mm) with inner radius Ri
and outer radius and are contained only in the volume named "volname."

Note that the old syntax (old as of r1188) still works, for backwards
compatibility. The old syntax is::

    /generator/pos/set Vx Vy Vz X Y Z Ro Ri

where points are contained only within the same volume as the point (Vx, Vy,
Vz).
