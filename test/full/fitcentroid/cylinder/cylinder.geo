{
name: "GEO",
index: "world",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "", // world volume has no mother
type: "box",
size: [20000.0, 20000.0, 20000.0], // mm, half-length
material: "rock",
invisible: 1,
}

{
name: "GEO",
index: "detector",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "world",
type: "tube",
r_max: 3500.0,
size_z: 3500.0,
position: [0.0, 0.0, 0.0],
material: "scintillator",
color: [0.4, 0.4, 0.6, 0.1],
}

// use an inner volume to speed up simulation.
{
name: "GEO",
index: "detector_inner",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max: 2500.0,
size_z: 2500.0,
position: [0.0, 0.0, 0.0],
material: "scintillator",
color: [0.4, 0.4, 0.6, 0.1],

}

{ 
name: "GEO", 
index: "pmts", 
valid_begin: [0, 0], 
valid_end: [0, 0], 
mother: "detector", 
type: "pmtarray", 
pmt_model: "r1408", 
pmt_detector_type: "idpmt",
sensitive_detector: "/mydet/pmt/inner", 
efficiency_correction: 1.027,  
pos_table: "PMTINFO", 
orientation: "manual",
//orient_point: [0.0, 0.0, 0.0], 
} 

