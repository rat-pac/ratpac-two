{
  name: "GEO",
  index: "world",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "",
  type: "box",
  size: [20000.0,20000.0,20000.0],
  material: "mirror",
}

{
  name: "GEO",
  index: "outer_vessel",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "world",
  type: "box",
  size: [10.0,10.0,10.0],
  position: [0.0, 0.0, 0.0],
  rotation: [0.0, 0.0, 0.0],
  material: "aluminum",
  color: [0.02,0.2,0.2,0.03],
}

{
  name: "GEO",
  index: "outer_tank",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "outer_vessel",
  type: "box",
  size: [9.0,9.0,9.0],
  position: [0.0, 0.0, 0.0],
  rotation: [0.0, 0.0, 0.0],
  material: "mirror",
  #color: [0.02,0.2,0.2,0.1],
}

// here we place nested tubes manually
{
  name: "GEO",
  index: "fiber_0_outer",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "outer_tank",
  type: "tube",
  r_max: 0.5,
  size_z: 49.5,
  position: [-5.0, 0.0, -5.0],
  rotation: [-90.0, 0.0, 0.0],
  material: "aluminum",
  color: [0.0,0.8,0.0,0.01],
}

{
  name: "GEO",
  index: "fiber_0_inner",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "fiber_0_outer",
  type: "tube",
  r_max: 0.485,
  size_z: 49.5,
  position: [0.0, 0.0, 0.0],
  rotation: [0.0, 0.0, 0.0],
  material: "glass",
  color: [0.0,0.8,0.0,0.05],
}

{
  name: "GEO",
  index: "fiber_0_core",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "fiber_0_inner",
  type: "tube",
  r_max: 0.47,
  size_z: 49.5,
  position: [0.0, 0.0, 0.0],
  rotation: [0.0, 0.0, 0.0],
  material: "mirror",
  color: [0.0,0.8,0.0,0.1],
}
{
  name: "GEO",
  index: "fiber_1_outer",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "outer_tank",
  type: "tube",
  r_max: 0.5,
  size_z: 29.5,
  position: [-5.0, 0.0, 5.0],
  rotation: [-90.0, 0.0, 0.0],
  material: "aluminum",
  color: [0.0,0.8,0.0,0.01],
}

{
  name: "GEO",
  index: "fiber_1_inner",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "fiber_1_outer",
  type: "tube",
  r_max: 0.485,
  size_z: 29.5,
  position: [0.0, 0.0, 0.0],
  rotation: [0.0, 0.0, 0.0],
  material: "glass",
  color: [0.0,0.8,0.0,0.05],
}

{
  name: "GEO",
  index: "fiber_1_core",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "fiber_1_inner",
  type: "tube",
  r_max: 0.47,
  size_z: 29.5,
  position: [0.0, 0.0, 0.0],
  rotation: [0.0, 0.0, 0.0],
  material: "mirror",
  color: [0.0,0.8,0.0,0.1],
}
// manual nested tubes ends here

// this table is the information
// about where to place the nested tubes
// in the array
{
name: "cable_pos",
valid_begin: [0],
valid_end: [0],
x: [5, 5]
y: [0, 0],
z: [5, -5],
dir_x: [0, 0],
dir_y: [1, 1],
dir_z: [0, 0],
Dz: [49.5, 29.5]
}

// this is the definition of the
// nested tube 'properties'
// note that we reference the pos_table
// by name, also the pos_table
// doesn't have to be in the .geo file
{
name: "GEO",
index: "fibers",
enable: 1,
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "outer_tank",
type: "nestedtubearray",
core_r: 0.47,
inner_r: 0.485,
outer_r: 0.5,
pos_table: "cable_pos",
orientation: "manual",
material_outer: "aluminum",
material_inner: "glass",
material_core: "mirror",
#drawstyle: "solid",
color: [0.8,0.0,0.0,0.8]
}
