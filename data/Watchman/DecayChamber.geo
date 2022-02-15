{
name: "GEO",
index: "sourcecan",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",

material: "stainless_steel",
position: [0, 0, 0],
invisible: 0,
r_max: 55.0,
size_z: 155.0,

color: [0.8, 0.8, 1.8, 0.8],
}

{
name: "GEO",
index: "sourceair",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "sourcecan",
type: "tube",

material: "air",
position: [0, 0, 0],
invisible: 0,
r_max: 50.0,
size_z: 150.0
}

{
name: "GEO",
index: "sourcescint",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "sourceair",
type: "tube",

material: "BGO_scint",
position: [0.0, 0.0, -98.5],
invisible: 0,
r_max: 50.0,
size_z: 48.5,

color: [0.3, 0.0, 0.8, 0.4],
}

{
name: "GEO",
index: "sourceblock",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "sourceair",
type: "tube",

material: "aluminum",
position: [0.0, 0.0, -148.5],
invisible: 0,
r_max: 20.0,
size_z: 1.5,

color: [0.8, 0.8, 0.8, 1.0],
}

{
name: "GEO",
index: "sourcepmt",
enable: 1,
valid_begin: [0, 0],
valid_end: [0, 0],

mother: "sourceair",
type: "pmtarray",
pmt_model: "calibrationPmt", //trigger pmt -- r7081pe
pmt_detector_type: "idpmt",
sensitive_detector: "/mydet/pmt/trigger", 
efficiency_correction: 1.000,
mu_metal: 0,
pos_table: "SourcePMT", //Not sure what this does
start_idx: 0,
end_index: 0,
orientation: "manual",

use_parent_coordinates: 1,
}
