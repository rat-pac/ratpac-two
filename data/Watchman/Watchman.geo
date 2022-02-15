{
name: "GEO",
index: "world",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "", // world volume has no mother
type: "box",
size: [20000.0, 20000.0, 20000.0], // mm, half-length
material: "air", //rock?
invisible: 1,
}

///////////////////// Define the rock volumes. Thin slab of rock is assumed ////////////////////////

//Create a 1-m rock layer around a cylindrical cavern
{
name: "GEO",
index: "rock_1",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "world", // world volume has no mother
type: "tube",
r_max: 14000.00, // changed to accommodate 0.5m-thick layer of concrete on walls (L. Kneale)
size_z: 13750.00,
position: [0.0, 0.0, 0.0], //this will allow for the concrete layer on the floor and not on the ceiling
material: "rock",
invisible: 1,
//color: [1.0,0.6,0.0,1.0],
//drawstyle: "solid"
}


//Create a 0.5m concrete layer on the walls and base
{
name: "GEO",
index: "concrete", // changed from "gunite" for updated design (L. Kneale)
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "rock_1",
type: "tube",
r_max: 13000.0, // changed to incorporate 0.5m layer of concrete on walls (L.Kneale) (cavern size is 25m i.e. 12.5m radius)
size_z: 12751.0, // the extraneous mm ensures that the cavern volume is enclosed by the "concrete" mother volume
position: [0.0, 0.0, -250.0], // this will give a concrete layer on the floor and not on the ceiling
material: "concrete", // changed from "gunite" (L. Kneale)
invisible: 1,
//color: [0.8,0.8,0.8,0.8],
//drawstyle: "solid"
}


//Create the cavern space between the tank and concrete
{
name: "GEO",
index: "cavern",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "concrete",
type: "tube",
r_max: 12500.0,
size_z: 12500.0,
position: [0.0, 0.0, 250.0],
material: "air",
invisible: 1,
}

////////////////////////////////// Define the rock volumes done.///////////////////////////////////


////////////////////////////////// Define detector properties.  ///////////////////////////////////

{
name: "GEO",
index: "tank",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "cavern",
type: "tube",
r_max: 10000.0,
size_z: 10000.0,
position: [0.0, 0.0, 0.0],
material: "stainless_steel",
color: [0.6,0.6,0.9,0.01],
drawstyle: "solid"
}

{
name: "GEO",
index: "detector",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "tank",
type: "tube",
r_max: 9984.125,
size_z: 9984.125, //half height, mm
position: [0.0, 0.0, 0.0],
material: "doped_water",
color: [0.2,0.2,0.9,0.2],
drawstyle: "solid"
}




//reflective tarp for veto region - including reflection properties
{
name: "GEO",
index: "white_sheet_side",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  7100.0,// These are guessed. Need a proper estimate
r_min:  7090.0,//
size_z: 7100.0,// These are guessed. Need a proper estimate
position: [0.0, 0.0, 0.0],
material: "polypropylene",
color: [0.9,0.9,0.9,0.3],
drawstyle: "solid",
}
{
//Bergevin: Set the interface were reflection can occur. Must make sure volume1 and volume2
//are in the correct order
name: "GEO",
index: "midsurface_white_sheet_side",
valid_begin: [0, 0],
valid_end: [0, 0],
invisible: 0, // omitted for visualization
mother: "white_sheet_side", //not used but needs to be a valid name, parent of 'a' and 'b' would be best choice
type: "border",
volume1: "detector",
volume2: "white_sheet_side",
reverse: 1, //0 only considers photons from a->b, 1 does both directions
surface: "reflective_tarp",
}



{
name: "GEO",
index: "white_sheet_top",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  6710.0,// These are guessed. Need a proper estimate
size_z: 10.0,// These are guessed. Need a proper estimate
position: [0.0, 0.0, 6705.0],
material: "polypropylene",
color: [0.9,0.9,0.9,0.3],
drawstyle: "solid",
}{
//Bergevin: Set the interface were reflection can occur. Must make sure volume1 and volume2
//are in the correct order
name: "GEO",
index: "midsurface_white_sheet_top",
valid_begin: [0, 0],
valid_end: [0, 0],
invisible: 0, // omitted for visualization
mother: "white_sheet_top", //not used but needs to be a valid name, parent of 'a' and 'b' would be best choice
type: "border",
volume1: "detector",
volume2: "white_sheet_top",
reverse: 1, //0 only considers photons from a->b, 1 does both directions
surface: "reflective_tarp",
}


/*
{
{name: "GEO",
index: "white_sheet_top",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  7100.0,// These are guessed. Need a proper estimate
size_z: 10.0,// These are guessed. Need a proper estimate
position: [0.0, 0.0, 7100.0],
material: "polypropylene",
color: [0.9,0.9,0.9,0.3],
drawstyle: "solid",
}
{
//Bergevin: Set the interface were reflection can occur. Must make sure volume1 and volume2
//are in the correct order
name: "GEO",
index: "midsurface_white_sheet_top",
valid_begin: [0, 0],
valid_end: [0, 0],
invisible: 0, // omitted for visualization
mother: "white_sheet_top", //not used but needs to be a valid name, parent of 'a' and 'b' would be best choice
type: "border",
volume1: "detector",
volume2: "white_sheet_top",
reverse: 1, //0 only considers photons from a->b, 1 does both directions
surface: "reflective_tarp",
}
*/

{
//Tomi Akindele based on the comments from SAM in Chris's Presentation to SAS subgroup leads
name:"GEO",
index: "Top_cap_framework",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  6700.0,
size_z: 100.0, //this values is actually 450, but it wont fit without breaking everything
position: [0.0, 0.0, 6902.5]
material: "stainless_steel",
color: [0.6,0.6,0.9,0.1],
drawstyle: "solid",
}

{
//Tomi	Akindele based on the comments from SAM	in Chris's Presentation	to SAS subgroup	leads
name:"GEO",
index: "Wall_support_truss_top",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  6900.0,
r_min: 6705.0, //these numbers interfere with the top cap framework
size_z: 100.0, //this values is actually 450, but it wont fit without breaking everything
position: [0.0, 0.0, 6902.5]
material: "stainless_steel",
color: [1.0,0.64,0.,0.1],
drawstyle: "solid",
}

{
//Tomi Akindele based on the comments from SAM in Chris's Presentation to SAS subgroup leads
name:"GEO",
index: "Bottom_cap_framework",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  6700.0,
size_z: 100.0, //this values is actually 450, but it wont fit without breaking everything
position: [0.0, 0.0, -6902.5]
material: "stainless_steel",
color: [0.6,0.6,0.9,0.1],
drawstyle: "solid",
}

{
//Tomi Akindele based on the comments from SAM in Chris's Presentation to SAS subgroup leads
name:"GEO",
index: "Wall_support_truss_bottom",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  6900.0,
r_min: 6705.0, //these numbers interfere with the top cap framework
size_z: 100.0, //this values is actually 450, but it wont fit without breaking everything
position: [0.0, 0.0, -6902.5]
material: "stainless_steel",
color: [1.0,0.64,0.,0.1],
drawstyle: "solid",
}

{
//Tomi Akindele based on the comments from SAM in Chris's Presentation to SAS subgroup leads
name: "GEO",
index: "Rod_assemblies",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  7000.0,// Some changes to just make this thing fit
r_min:  6900.0,
size_z: 6700.0,
position: [0.0, 0.0, 0.0],
material: "stainless_steel",
color: [0.6,0.6,0.9,0.1],
drawstyle: "solid",
}
{
//Tomi Akindele based on the comments from SAM in Chris's Presentation to SAS subgroup leads
// Bergevin: Switch x and y for presentation purposes
name: "GEO",
index: "Bottom_cap_standoff_frame_0",//center support
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "box",
size: [7000.0, 200.0,  1000.0],//x,y,z
position: [0.0, 0.0, -8200.0],
material: "stainless_steel",
color: [0.6,0.6,0.9,0.1],
drawstyle: "solid",
}

{
//Tomi Akindele based on the comments from SAM in Chris's Presentation to SAS subgroup leads
name: "GEO",
index: "Bottom_cap_standoff_frame_1",//one off 
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "box",
size: [6800.0,200.0,  1000.0],//x,y,z
position: [ 0.0,3000.0, -8200.0],
material: "stainless_steel",
color: [0.6,0.6,0.9,0.1],
drawstyle: "solid",
}
{
//Tomi Akindele based on the comments from SAM in Chris's Presentation to SAS subgroup leads
name: "GEO",
index: "Bottom_cap_standoff_frame_2",//one off	
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "box",
size: [6800.0,200.0,  1000.0],//x,y,z
position: [0.0,-3000.0,  -8200.0],
material: "stainless_steel",
color: [0.6,0.6,0.9,0.1],
drawstyle: "solid",
}
{
//Tomi Akindele based on the comments from SAM in Chris's Presentation to SAS subgroup leads
name: "GEO",
index: "Bottom_cap_standoff_frame_3",//two off	
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "box",
size: [4400.0,200.0,  1000.0],//x,y,z
position: [ 0.0,5800.0, -8200.0],
material: "stainless_steel",
color: [0.6,0.6,0.9,0.1],
drawstyle: "solid",
}
{
//Tomi Akindele based on the comments from SAM in Chris's Presentation to SAS subgroup leads
name: "GEO",
index: "Bottom_cap_standoff_frame_4",//three off
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "box",
size: [4400.0,200.0,  1000.0],//x,y,z
position: [0.0,-5800.0,  -8200.0],
material: "stainless_steel",
color: [0.6,0.6,0.9,0.1],
drawstyle: "solid",
}
{
name: "GEO",
index: "white_sheet_bottom",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  7100.0,// These are guessed. Need a proper estimate
size_z: 10.0,// These are guessed. Need a proper estimate
position: [0.0, 0.0, -7100.0],
material: "polypropylene",
color: [0.9,0.9,0.9,0.1],
drawstyle: "solid",
}
{
//Bergevin: Set the interface were reflection can occur. Must make sure volume1 and volume2
//are in the correct order
name: "GEO",
index: "midsurface_white_sheet_bottom",
valid_begin: [0, 0],
valid_end: [0, 0],
invisible: 0, // omitted for visualization
mother: "white_sheet_bottom", //not used but needs to be a valid name, parent of 'a' and 'b' would be best choice
type: "border",
volume1: "detector",
volume2: "white_sheet_bottom",
reverse: 1, //0 only considers photons from a->b, 1 does both directions
surface: "reflective_tarp",
}




//reflective tarp for veto region - including reflection properties
{
name: "GEO",
index: "white_sheet_tank_side",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  9970.0,// These are guessed. Need a proper estimate
r_min:  9960.0,//
size_z: 9970.0,// These are guessed. Need a proper estimate
position: [0.0, 0.0, 0.0],
material: "polypropylene",
color: [0.9,0.9,0.2,0.1],
drawstyle: "solid",
}
{
//Bergevin: Set the interface were reflection can occur. Must make sure volume1 and volume2
//are in the correct order
name: "GEO",
index: "midsurface_white_sheet_tank_side",
valid_begin: [0, 0],
valid_end: [0, 0],
invisible: 0, // omitted for visualization
mother: "white_sheet_tank_side", //not used but needs to be a valid name, parent of 'a' and 'b' would be best choice
type: "border",
volume1: "detector",
volume2: "white_sheet_tank_side",
reverse: 1, //0 only considers photons from a->b, 1 does both directions
surface: "reflective_tarp",
}
{name: "GEO",
index: "white_sheet_tank_top",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  9965.0,// These are guessed. Need a proper estimate
size_z: 10.0,// These are guessed. Need a proper estimate
position: [0.0, 0.0, 9970.0],
material: "polypropylene",
color: [0.9,0.9,0.2,0.1],
drawstyle: "solid",
}
{
//Bergevin: Set the interface were reflection can occur. Must make sure volume1 and volume2
//are in the correct order
name: "GEO",
index: "midsurface_white_sheet_tank_top",
valid_begin: [0, 0],
valid_end: [0, 0],
invisible: 0, // omitted for visualization
mother: "white_sheet_tank_top", //not used but needs to be a valid name, parent of 'a' and 'b' would be best choice
type: "border",
volume1: "detector",
volume2: "white_sheet_tank_top",
reverse: 1, //0 only considers photons from a->b, 1 does both directions
surface: "reflective_tarp",
}
{
name: "GEO",
index: "white_sheet_tank_bottom",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  9965.0,// These are guessed. Need a proper estimate
size_z: 10.0,// These are guessed. Need a proper estimate
position: [0.0, 0.0, -9970.0],
material: "polypropylene",
color: [0.9,0.9,0.2,0.1],
drawstyle: "solid",
}
{
//Bergevin: Set the interface were reflection can occur. Must make sure volume1 and volume2
//are in the correct order
name: "GEO",
index: "midsurface_white_sheet_tank_bottom",
valid_begin: [0, 0],
valid_end: [0, 0],
invisible: 0, // omitted for visualization
mother: "white_sheet_tank_bottom", //not used but needs to be a valid name, parent of 'a' and 'b' would be best choice
type: "border",
volume1: "detector",
volume2: "white_sheet_tank_bottom",
reverse: 1, //0 only considers photons from a->b, 1 does both directions
surface: "reflective_tarp",
}

//non-reflective tarp for inner detector - including non-reflective properties
{
name: "GEO",
index: "black_sheet_side",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  6710.0,// These are guessed. Need a proper estimate
r_min:  6700.0,//
size_z: 6700.0,// These are guessed. Need a proper estimate
position: [0.0, 0.0, 0.0],
material: "polypropylene",
color: [0.,0.,0.,1.0],
drawstyle: "solid",
}
{
//Bergevin: Set the interface were reflection can occur. Must make sure volume1 and volume2
//are in the correct order
name: "GEO",
index: "midsurface_black_sheet_side",
valid_begin: [0, 0],
valid_end: [0, 0],
invisible: 0, // omitted for visualization
mother: "black_sheet_side", //not used but needs to be a valid name, parent of 'a' and 'b' would be best choice
type: "border",
volume1: "detector",
volume2: "black_sheet_side",
reverse: 1, //0 only considers photons from a->b, 1 does both directions
surface: "nonreflective_tarp",
}
{
name: "GEO",
index: "black_sheet_top",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  6710.0,// These are guessed. Need a proper estimate
size_z: 10.0,// These are guessed. Need a proper estimate
position: [0.0, 0.0, 6705.0],
material: "polypropylene",
color: [0.,0.,0.,1.0],
drawstyle: "solid",
}{
//Bergevin: Set the interface were reflection can occur. Must make sure volume1 and volume2
//are in the correct order
name: "GEO",
index: "midsurface_black_sheet_top",
valid_begin: [0, 0],
valid_end: [0, 0],
invisible: 0, // omitted for visualization
mother: "black_sheet_top", //not used but needs to be a valid name, parent of 'a' and 'b' would be best choice
type: "border",
volume1: "detector",
volume2: "black_sheet_top",
reverse: 1, //0 only considers photons from a->b, 1 does both directions
surface: "nonreflective_tarp",
}
{
name: "GEO",
index: "black_sheet_bottom",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max:  6710.0,// These are guessed. Need a proper estimate
size_z: 10.0,// These are guessed. Need a proper estimate
position: [0.0, 0.0, -6705.0],
material: "polypropylene",
color: [0.,0.,0.,1.0],
drawstyle: "solid",
}
{
//Bergevin: Set the interface were reflection can occur. Must make sure volume1 and volume2
//are in the correct order
name: "GEO",
index: "midsurface_black_sheet_bottom",
valid_begin: [0, 0],
valid_end: [0, 0],
invisible: 0, // omitted for visualization
mother: "black_sheet_bottom", //not used but needs to be a valid name, parent of 'a' and 'b' would be best choice
type: "border",
volume1: "detector",
volume2: "black_sheet_bottom",
reverse: 1, //0 only considers photons from a->b, 1 does both directions
surface: "nonreflective_tarp",
}



/*  This needs to be redone by Yan-Jie and Brian to follow the PSUP model
{ //position table for hold-up cables
name: "cable_pos",
valid_begin: [0, 0],
valid_end: [0, 0],
x: [6406.35d,6392.63d,6351.54d,6283.25d,6188.06d,6066.37d,5918.7d,5745.68d,5548.06d,5326.69d,5082.5d,4816.55d,4529.97d,4224.d,3899.94d,3559.18d,3203.18d,2833.46d,2451.6d,2059.25d,1658.09d,1249.82d,836.196d,418.995d,0.d,-418.995d,-836.196d,-1249.82d,-1658.09d,-2059.25d,-2451.6d,-2833.46d,-3203.18d,-3559.18d,-3899.94d,-4224.d,-4529.97d,-4816.55d,-5082.5d,-5326.69d,-5548.06d,-5745.68d,-5918.7d,-6066.37d,-6188.06d,-6283.25d,-6351.54d,-6392.63d,-6406.35d,-6392.63d,-6351.54d,-6283.25d,-6188.06d,-6066.37d,-5918.7d,-5745.68d,-5548.06d,-5326.69d,-5082.5d,-4816.55d,-4529.97d,-4224.d,-3899.94d,-3559.18d,-3203.18d,-2833.46d,-2451.6d,-2059.25d,-1658.09d,-1249.82d,-836.196d,-418.995d,0.d,418.995d,836.196d,1249.82d,1658.09d,2059.25d,2451.6d,2833.46d,3203.18d,3559.18d,3899.94d,4224.d,4529.97d,4816.55d,5082.5d,5326.69d,5548.06d,5745.68d,5918.7d,6066.37d,6188.06d,6283.25d,6351.54d,6392.63d,],
y: [0.d,418.995d,836.196d,1249.82d,1658.09d,2059.25d,2451.6d,2833.46d,3203.18d,3559.18d,3899.94d,4224.d,4529.97d,4816.55d,5082.5d,5326.69d,5548.06d,5745.68d,5918.7d,6066.37d,6188.06d,6283.25d,6351.54d,6392.63d,6406.35d,6392.63d,6351.54d,6283.25d,6188.06d,6066.37d,5918.7d,5745.68d,5548.06d,5326.69d,5082.5d,4816.55d,4529.97d,4224.d,3899.94d,3559.18d,3203.18d,2833.46d,2451.6d,2059.25d,1658.09d,1249.82d,836.196d,418.995d,0.d,-418.995d,-836.196d,-1249.82d,-1658.09d,-2059.25d,-2451.6d,-2833.46d,-3203.18d,-3559.18d,-3899.94d,-4224.d,-4529.97d,-4816.55d,-5082.5d,-5326.69d,-5548.06d,-5745.68d,-5918.7d,-6066.37d,-6188.06d,-6283.25d,-6351.54d,-6392.63d,-6406.35d,-6392.63d,-6351.54d,-6283.25d,-6188.06d,-6066.37d,-5918.7d,-5745.68d,-5548.06d,-5326.69d,-5082.5d,-4816.55d,-4529.97d,-4224.d,-3899.94d,-3559.18d,-3203.18d,-2833.46d,-2451.6d,-2059.25d,-1658.09d,-1249.82d,-836.196d,-418.995d,],
z: [0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,],
dir_x: [0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,],
dir_y: [0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,0.d,],
dir_z: [1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,1.d,],
}

{
name: "GEO",
index: "cables",
enable: 1,
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tubearray",
r_max: 9.525,
size_z: 7984.125,
pos_table: "cable_pos",
orientation: "manual",
material: "stainless_steel",
drawstyle: "solid",
color: [0.2,0.2,0.2,0.1],
}
*/


{
name: "inner_vis",
valid_begin: [0, 0],
valid_end: [0, 0],
color: [0.0,0.5, 0.0, 0.2],
}

{
name: "veto_vis",
valid_begin: [0, 0],
valid_end: [0, 0],
color: [1.0, 0.0, 1.0, 0.3],
}

{
name: "GEO",
index: "shield",
enable: 1,
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "watchmanshield", //see the geo factory

//builds pmt backs/covers
pmtinfo_table: "PMTINFO",
back_semi_x: 152.5,
back_semi_y: 152.5,
back_semi_z: 301.5, //this should perhaps be a little larger
back_thickness: 3.175,
back_material: "polypropylene",
orientation_inner: "manual", //should match "inner_pmts" orientation
orient_point_inner: [0.,0.,0.], //only used if orientation_inner is "point"
inner_start: 0,
inner_len: 3258, //set to 0 to prevent building covers
inner_back_surface: "black_water",
inner_back_vis: "inner_vis",
orientation_veto: "manual", //should match "veto_pmts" orientation
orient_point_veto: [0.,0.,0.], //only used if orientation_veto is "point"
veto_start: 3258,
veto_len: 296, //set to 0 to prevent building covers
veto_back_surface: "black_water",
veto_back_vis: "veto_vis",

//properties to define the shield
detector_size_z: 20000.0,//Full Height
detector_size_d: 20000.0,//Full Diameter
veto_thickness_r: 3300.0,
veto_thickness_z: 3300.0,
steel_thickness: 1.5875,
cols: 84,
rows: 26,
frame_material: "stainless_steel",
inside_surface: "black_water",
outside_surface: "white_water",

drawstyle: "solid",
position: [0.0, 0.0, 0.0],
color: [0.1, 0.8, 0.1, 0.01],
}

/* Will not work with WATCHMAKERS as it is now
// Fiducial defined as separate geometry component a la baccarat
{
name: "GEO",
index: "fiducial",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "tube",
r_max: 5420.0,
size_z: 5420.0,
position: [0.0, 0.0, 0.0],
material: "doped_water",
color: [0.2,0.2,0.2,0.1],
drawstyle: "solid",
invisible: 1
}*/

{
name: "GEO",
index: "inner_pmts",
enable: 1,
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "pmtarray",
end_idx: 3257, //idx of the last pmt
start_idx: 0, //idx of the first pmt
pmt_model: "r7081pe",
mu_metal: 0,
mu_metal_material: "aluminum",
mu_metal_surface: "aluminum",
light_cone: 0,
light_cone_material: "aluminum",
light_cone_surface: "aluminum",
light_cone_length: 17.5,
light_cone_innerradius: 12.65,
light_cone_outerradius: 21,
light_cone_thickness: 0.2,
black_sheet_offset: 300.0, //30 cm default black tarp offset
black_sheet_thickness: 10.0, //1 cm default black tarp thickness 
pmt_detector_type: "idpmt",
sensitive_detector: "/mydet/pmt/inner",
efficiency_correction: 0.90000,
pos_table: "PMTINFO", //generated by positions.nb
orientation: "manual",
orient_point: [0.,0.,0.],
color: [0.3,0.5, 0.0, 0.2],
}

{
name: "GEO",
index: "veto_pmts",
enable: 1,
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector",
type: "pmtarray",
end_idx: 3553, //idx of the last pmt
start_idx: 3258, //idx of the first pmt
pmt_model: "r7081pe",
mu_metal: 0,
light_cone: 0,
pmt_detector_type: "idpmt",
sensitive_detector: "/mydet/pmt/veto",
efficiency_correction: 0.90000,
pos_table: "PMTINFO", //generated by positions.nb
orientation: "manual",
orient_point: [0.,0.,0.],
}
