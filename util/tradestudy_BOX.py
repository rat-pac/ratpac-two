'''Create a PMTINFO RATDB table with positions of PMTs arranged in a box.
Must be run from the util folder. Will create a files in the data folder.
'''
import numpy as np
import math
import os

### Default values to change ratdb geometry files

xPMT    = 3065.0
yPMT    = 24065.0 ## 50-m tank
yPMT    = 39065.0 ## 80-m tank
zPMT    = 3065.0

dFIDVol = -150.0 ## Arbitrary 1m buffer
tFIDVol = 0.0
dPSUP   = 385    
tPSUP   = 6.0
tBSHEET = 5.0
dTANK   = 535.0  
tTANK   = 50.0
oTANK   = 200.
dIBEAM  = 500. 
tIBEAM  = 27.0 
dAIR    = 1400.0 
dCONC   = 100.0
tCONC   = 25000.0
dROCK   = 2000.0  


## Values to change for PMT arrangement. (PMTINFO)
photocoverage = 0.109
photocoverage = 0.157
photocoverage = 0.205
pmtRad        = 126.5


def geoFile(xPMT = 3498.125,  yPMT = 23498.125, zPMT = 3498.125,\
dFIDVol = -1000.0,\
tFIDVol = 0.0,\
dPSUP   = 100.0,\
tPSUP   = 10.0,\
tBSHEET = 5.0,\
dTANK   = 501.875,    
tTANK   = 500.0,\
oTANK   = 200.0,\
dAIR    = 1000.0 ,\
dIBEAM  = 500. ,\
tIBEAM  = 27.0 ,\
dCONC   = 500.0,\
tCONC   = 25000.0,\
dROCK   = 2000.0,\
pmtCnt  =  4000.0   ):                            
## dtank With respect to PMTs

    return  f"""{{
name: "GEO",
index: "world",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "", // world volume has no mother
type: "box",
size: [{xPMT+dTANK+dAIR+dROCK+dAIR+xPMT}, {yPMT+dTANK+dAIR+dROCK+dAIR+xPMT}, {zPMT+dTANK+dAIR+dROCK+dAIR+xPMT}], // mm, half-length
position: [0.0, 0.0, 0.0],
material: "air", //rock?
color: [0.85, 0.72, 1.0, 0.5],
invisible: 0,
}}

///////////////////// Define the rock volumes. Thin slab of rock is assumed ////////////////////////

//Create a 1-m rock layer around a cylindrical cavern
{{
name: "GEO",
index: "rock_1",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "world", // world volume has no mother
type: "box",
//size: [7000.0, 27000.0, 7000.0], // mm, half-length
size: [{xPMT+dTANK+dAIR+dROCK}, {yPMT+dTANK+dAIR+dROCK}, {zPMT+dTANK+dAIR+dROCK}],
position: [0.0, 0.0, {dAIR-tIBEAM-tTANK}], //this will allow for the concrete layer on the floor and not on the ceiling
material: "rock",
color: [0.43, 0.27, 0.13, 1.0],
invisible: 0,
//drawstyle: "solid"
}}
//Create a 0.5m concrete layer on the walls and base
{{
name: "GEO",
index: "rock_2",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "rock_1",
type: "box",
//size: [5500.0, 26500.0, 5500.0], // mm, half-length
size: [{xPMT+dTANK+dAIR+dCONC}, {yPMT+dTANK+dAIR+dCONC}, {zPMT+dTANK+dAIR+dCONC}], 
position: [0.0, 0.0, 0.0], // this will give a concrete layer on the floor and not on the ceiling
material: "rock", // changed from "gunite" (L. Kneale)
color: [0.43, 0.27, 0.13, 1.0],
invisible: 0,
//color: [0.8,0.8,0.8,0.8],
//drawstyle: "solid"
}}
//Create the cavern space between the tank and concrete
{{
name: "GEO",
index: "cavern_1",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "rock_2",
type: "box",
//size: [5000.0, 26000.0, 5000.0], // mm, half-length
size: [{xPMT+dTANK+dAIR}, {yPMT+dTANK+dAIR}, {zPMT+dTANK+dAIR}], 
position: [0.0, 0.0, 0.0],
material: "air",
invisible: 0,
color: [0.85, 0.72, 1.0, 0.5],
}}
{{
name: "GEO",
index: "ibeam",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "cavern_1",
type: "box",
//size: [4500.0, 24500.0, 4500.0], // mm, half-length
size: [{xPMT+dTANK+dAIR-dIBEAM+tIBEAM}, {yPMT+dTANK+dAIR-dIBEAM+tIBEAM}, {zPMT+dTANK+dAIR-dIBEAM+tIBEAM}], 
position: [0.0, 0.0,{-(dIBEAM-tIBEAM)}],
material: "stainless_steel",
color: [0.96,0.95,0.27,1.0],
drawstyle: "solid"
}}
{{
name: "GEO",
index: "cavern_2",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "ibeam",
type: "box",
//size: [5000.0, 26000.0, 5000.0], // mm, half-length
size: [{xPMT+dTANK+dAIR-dIBEAM}, {yPMT+dTANK+dAIR-dIBEAM}, {zPMT+dTANK+dAIR-dIBEAM}], 
position: [0.0, 0.0, 0.0],
material: "air",
color: [0.85, 0.72, 1.0, 0.5],
invisible: 0,
}}
////////////////////////////////// Define the rock volumes done.///////////////////////////////////
{{
name: "GEO",
index: "tank",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "cavern_2",
type: "box",
//size: [4500.0, 24500.0, 4500.0], // mm, half-length
size: [{xPMT+dTANK+tTANK}, {yPMT+dTANK+tTANK}, {zPMT+dTANK+tTANK+oTANK}], 
position: [0.0, 0.0, {oTANK-dAIR+dIBEAM+tTANK}],
material: "stainless_steel",
color: [0.43,0.70,0.90,1.0],
drawstyle: "solid"
}}
{{
name: "GEO",
index: "detector_veto1",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "tank",
type: "box",
//size: [4498.125, 24498.125, 4498.125], // mm, half-length
size: [{xPMT+dTANK}, {yPMT+dTANK}, {zPMT+dTANK+oTANK}],
position: [0.0, 0.0, 0.0],
material: "doped_water",
color: [0.2,0.2,0.9,0.2],
drawstyle: "solid"
}}
{{
name: "GEO",
index: "psup",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector_veto1",
type: "box",
//size: [4498.125, 24498.125, 4498.125], // mm, half-length
size: [{xPMT+dPSUP+tPSUP}, {yPMT+dPSUP+tPSUP}, {zPMT+dPSUP+tPSUP}],
position: [0.0, 0.0, {-oTANK}],
material: "stainless_steel",
color: [0.0,0.5,0.18,1.0],
drawstyle: "solid"
}}
{{
name: "GEO",
index: "detector_veto2",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "psup",
type: "box",
//size: [4498.125, 24498.125, 4498.125], // mm, half-length
size: [{xPMT+dPSUP}, {yPMT+dPSUP}, {zPMT+dPSUP}],
position: [0.0, 0.0, 0.0],
material: "doped_water",
color: [0.2,0.2,0.9,0.2],
drawstyle: "solid"
}}
{{
name: "GEO",
index: "black_sheet",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector_veto2",
type: "box",
//size: [3508.125, 23508.125, 3508.125], // mm, half-length
size: [{xPMT+tBSHEET},{yPMT+tBSHEET},{zPMT+tBSHEET}],
position: [0.0, 0.0, 0.0],
material: "polypropylene",
color: [0.,0.,0.,1.0],
drawstyle: "solid",
}}
{{
name: "GEO",
index: "detector_target_gb", // gb: gamma buffer
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "black_sheet",
type: "box",
//size: [3498.125, 23498.125, 3498.125] // mm, half-length
size: [{xPMT},{yPMT},{zPMT}],
position: [0.0, 0.0, 0.0],
material: "doped_water",
color: [0.2,0.2,0.9,0.2],
drawstyle: "solid"
}}
{{
name: "GEO",
index: "detector_target_fv",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector_target_gb",
type: "box",
//size: [3498.125, 23498.125, 3498.125] // mm, half-length
size: [{xPMT+dFIDVol},{yPMT+dFIDVol},{zPMT+dFIDVol}],
position: [0.0, 0.0, 0.0],
material: "doped_water",
color: [0.2,0.2,0.9,0.2],
drawstyle: "solid"
}}
{{
//Bergevin: Set the interface were reflection can occur. Must make sure volume1 and volume2
//are in the correct order
name: "GEO",
index: "midsurface_black_sheet",
valid_begin: [0, 0],
valid_end: [0, 0],
invisible: 0, // omitted for visualization
mother: "black_sheet", //not used but needs to be a valid name, parent of 'a' and 'b' would be best choice
type: "border",
volume1: "detector_target_gb",
volume2: "black_sheet",
reverse: 1, //0 only considers photons from a->b, 1 does both directions
surface: "nonreflective_tarp",
}}

{{
name: "GEO",
index: "inner_pmts",
enable: 1,
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "detector_target_gb",
type: "pmtarray",
end_idx: {int(pmtCnt-1)}, //idx of the last pmt
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
}}
"""


def square(_x=3498.125, _y=23498.125, _z=3498.125, photocoverage=0.205, pmtRad = 126.5):
    pmtArea = 3.14159265359*pmtRad*pmtRad
    length = np.sqrt(pmtArea/photocoverage)
   
    _xL = 2.0*_x/length
    _yL = 2.0*_y/length
    _zL = 2.0*_z/length
  
    print('Number of spacing:',_xL,_yL,_zL)

    _nXCorr = 2.0*_x/length/math.floor(2.0*_x/length)
    _nYCorr = 2.0*_y/length/math.floor(2.0*_y/length)
    _nZCorr = 2.0*_z/length/math.floor(2.0*_z/length)

 
    nX = int(math.floor(2.0*_x/length))
    nY = int(math.floor(2.0*_y/length))
    nZ = int(math.floor(2.0*_z/length)) 

     
    
    cnt = 0
    x,y,z,dx,dy,dz,type = [],[],[],[],[],[],[]
    
    xEven = (nX%2==0)
    yEven = (nY%2==0)
    zEven = (nZ%2==0)
    print('//Length:',length, '(nX,nY,nZ):',nX, nY, nZ, 'Even:', xEven, yEven, zEven,'lengthCorrection:',_nXCorr, _nYCorr,_nZCorr)
    
    _lx = -_x + length/2.0*_nXCorr
    _ly = -_y + length/2.0*_nYCorr
    
    for __x in range(nX):
        #_lx += length*_nXCorr
        #_ly = -_y + length/2.0*_nYCorr
        for __y in range(nY):
            #_ly += length*_nYCorr 
            x.append(_lx)
            y.append(_ly)
            z.append(_z )
            dx.append(0.0)
            dy.append(0.0)
            dz.append(-1.0)
            type.append(1)
            x.append(_lx)
            y.append(_ly)
            z.append(-_z)
            dx.append(0.0)
            dy.append(0.0)
            dz.append(1.0)
            type.append(1)
            cnt+=2
            _ly += length*_nYCorr
            #print( '(',__x,  __y,'): (',_lx,_ly,_z,')',length) 
            #print( '(',__x,  __y,'): (',_lx,_ly,-_z,')',length)
        _lx += length*_nXCorr
        _ly = -_y + length/2.0*_nYCorr

    _lx = -_x + length/2.0*_nXCorr
    _lz = -_z + length/2.0*_nZCorr

    for __x in range(nX):
        ##_lx += length*_nXCorr
        ##_lz = -_z + length/2.0*_nZCorr
        for __z in range(nZ):
            #_lz += length*_nZCorr
            x.append(_lx)
            y.append(_y)
            z.append(_lz)
            dx.append(0.0)
            dy.append(-1.0)
            dz.append(0.0)
            type.append(1)
            x.append(_lx)
            y.append(-_y)
            z.append(_lz)
            dx.append(0.0)
            dy.append(1.0)
            dz.append(0.0)
            type.append(1)
            cnt+=2
            _lz += length*_nZCorr
            #print( '(',__x,  __y,'): (',_lx,_ly,_z,')',length) 
            #print( '(',__x,  __y,'): (',_lx,_ly,-_z,')',length)
        _lx += length*_nXCorr
        _lz = -_z + length/2.0*_nZCorr 

    _ly = -_y + length/2.0*_nYCorr
    _lz = -_z + length/2.0*_nZCorr
    for __y in range(nY):
        for __z in range(nZ):
            #_lz += length*_nZCorr
            x.append(_x)
            y.append(_ly)
            z.append(_lz)
            dx.append(-1.0)
            dy.append(0.0)
            dz.append(0.0)
            type.append(1)
            x.append(-_x)
            y.append(_ly)
            z.append(_lz)
            dx.append(1.0)
            dy.append(0.0)
            dz.append(0.0)
            type.append(1)
            cnt+=2
            _lz += length*_nZCorr
#            print( '(',__x,  __y,'): (',_lx,_ly,_z,')',length) 
#            print( '(',__x,  __y,'): (',_lx,_ly,-_z,')',length)
        _ly += length*_nYCorr
        _lz = -_z + length/2.0*_nZCorr 


    pmt_info = "{\n"
    pmt_info += f"//// Total number of inner PMTs : {cnt}\n"
    pmt_info += f"//// Total number of veto PMTs : 0\n"
    pmt_info += f"\"name\": \"PMTINFO\",\n"
    pmt_info += f"\"valid_begin\": [0, 0],\n"
    pmt_info += f"\"valid_end\": [0, 0],\n"
    pmt_info += f"\"x\":     {x},\n"
    pmt_info += f"\"y\":     {y},\n"
    pmt_info += f"\"z\":     {z},\n"
    pmt_info += f"\"dir_x\": {dx},\n"
    pmt_info += f"\"dir_y\": {dy},\n"
    pmt_info += f"\"dir_z\": {dz},\n"
    pmt_info += f"\"type\":  {type},\n"
    pmt_info += "}"
    
    return pmt_info,cnt
    #return x,y,z,dx,dy,dz,type,cnt


#####  main code ###########

_pmtinfo,cnt = square(_x=xPMT, _y=yPMT, _z=zPMT,\
photocoverage=photocoverage, pmtRad = pmtRad)

_geoFile = geoFile(xPMT = xPMT,  yPMT = yPMT, zPMT = zPMT,dFIDVol = dFIDVol,tFIDVol = tFIDVol,dPSUP   = dPSUP,tPSUP = tPSUP,\
tBSHEET = tBSHEET, dTANK = dTANK,tTANK = tTANK, oTANK = oTANK, dAIR = dAIR ,dCONC = dCONC,tCONC = tCONC, dROCK = dROCK, pmtCnt = cnt )

#print(_geoFile)
#print()
#print(_pmtinfo)

_str1 = "../data/"
_str2 = f"Watchman_letterbox_{int(np.ceil((xPMT+dTANK)*2.0/1000))}m_{int(np.ceil((yPMT+dTANK)*2.0/1000))}m_{int(np.ceil((zPMT+dTANK)*2.0/1000))}m_{int(photocoverage*100)}pct"

try:
    os.mkdir(f"{_str1}{_str2}")
    print('Created', f"{_str1}{_str2}")
except OSError as error:  
    print(error)   

geofile = open(f"{_str1}{_str2}/{_str2}.geo","w+")
geofile.writelines(_geoFile)
geofile.close

pmtfile = open(f"{_str1}{_str2}/PMTINFO.ratdb","w+")
pmtfile.writelines(_pmtinfo)
pmtfile.close

surfaceArea =  2.0*(2.*xPMT)*(2*yPMT) + 2.0*(2.*xPMT)*(2.*zPMT) + 2.0*(2.0*yPMT)*(2.*zPMT)
pmtArea     = float(cnt)*3.14159265359*pmtRad*pmtRad

print("//// Total number of inner PMTs : ",cnt)
print("//// Photocoverage (%) : ",photocoverage*100.)
print("//// Actual Photocoverage (%) :", pmtArea/surfaceArea)
print("//// Detector height (m) : ",(xPMT+dTANK)*2.0/1000.)
print("//// Detector length (m) : ",(yPMT+dTANK)*2.0/1000.)

print("////")

