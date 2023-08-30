led
'''
::

    /generator/add led POSITION:TIME

Creates a new LED generator using the position and time generators. The LED
generator can simulate any number of independent LEDs, each with a predefined
wavelength spectrum, intensity, timing, and angular distribution. The LED generator
can be controlled through its corresponding ratdb file ``LED.ratdb``. The parameters
of the ratdb file are described in the following table:

table::

    {
        name: "LED"
        index: "default",
        valid_begin: [0,0],
        valid_end: [0,0],
        
        intensity: 100,
        // When using multiples LEDs in sequence, (x, y, z) are their positions.
        x: [1.0],
        y: [1.0],
        z: [1.0],
        wavelength: [100.0, 200.0, 300.0, 400.0, 500.0, 600.0],
        // intensity_mode: {"single", "chain"}
        intensity_mode: "single",
        // time_mode: {"unif", "dist"}
        time_mode: "unif",
        // angle_mode: {"iso", "dist", "multidist"}
        angle_mode: "iso",
        // wl_mode: {"mono", "dist"}
        wl_mode: "mono",
        
        // Timing distribution if "dist"
        dist_time: [0.0, 25.0, 50.0],
        dist_time_intensity: [1.0, 2.0, 1.0],
        
        // Angular distribution if !iso -- radians?
        dist_angle: [0.0, 1.0],
        dist_angle_intensity: [1.0, 1.0],
        // Angular distributions if multidist
        n_ang_dists: 1,
        dist_angle0: [0.0, 2.0],
        dist_angle_intensity0: [1.0, 1.0],
        
        // Wavelength distribution if !mono
        dist_wl: [300.0, 800.0],
        dist_wl_intensity: [1.0, 1.0],
    }
