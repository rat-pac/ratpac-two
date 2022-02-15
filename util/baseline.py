'''Create a PMTINFO RATDB table with positions of PMTs arranged on a
cylinder.

'''

import numpy as np
import math


def topcap(height,rangeX,rangeY,_type,inv = 1.0, inner = 1.0,spacing = 500.,delta = 250.,tolerance = 200.):
    
    x,y = [],[]
    cnt = 0
    for _i in range(rangeX):
        for _j in range(rangeY):
            _x,_y = (_i*spacing + delta),(_j*spacing + delta)
            if np.sqrt(_x*_x+_y*_y)< height - tolerance:  #Asummes right cylinder such that Height = radius
                if (_i == 0 and _j == 0) :
                    a =1
                    #print("Skipping PMTs arround entry port")
                elif ((_i == 7 and _j == 7) or (_i == 8 and _j == 8)) and inner == 1:
                    #print("Skipping top quandrant for ports")
                    x.append(-_x)
                    y.append( _y)
                    x.append( _x)
                    y.append(-_y)
                    x.append(-_x)
                    y.append(-_y)
                    cnt+=3
                else:
                    x.append( _x)
                    y.append( _y)
                    x.append(-_x)
                    y.append( _y)
                    x.append( _x)
                    y.append(-_y)
                    x.append(-_x)
                    y.append(-_y)
                    cnt+=4

    
    z,dx,dy,dz,type = [],[],[],[],[]
    for i in range(cnt):
        z.append(height)
        dx.append(0.0)
        dy.append(0.0)
        dz.append(-1.0*inv)
        type.append(_type)

#    print(cnt)
    return x,y,z,dx,dy,dz,type,cnt




def bottomcap(height,rangeX,rangeY,_type,inv = 1.0,spacing = 500.,delta = 250.,tolerance = 200.):
    
    x,y = [],[]
    cnt = 0
    for _i in range(rangeX):
        for _j in range(rangeY):
            _x,_y = (_i*spacing + delta),(_j*spacing + delta)
            if np.sqrt(_x*_x+_y*_y)< height -tolerance:  #Asummes right cylinder such that Height = radius
                x.append( _x)
                y.append( _y)
                x.append(-_x)
                y.append( _y)
                x.append( _x)
                y.append(-_y)
                x.append(-_x)
                y.append(-_y)
                cnt+=4

    
    z,dx,dy,dz,type = [],[],[],[],[]
    for i in range(cnt):
        z.append(-height)
        dx.append(0.0)
        dy.append(0.0)
        dz.append(1.0*inv)
        type.append(_type)

#    print(cnt)
    return x,y,z,dx,dy,dz,type,cnt



def sidePMTs(radius,collums,nRings,_type,inv = 1.0,spacing = 500.,delta = 250.):
    
    x,y,z,dx,dy,dz,type = [],[],[],[],[],[],[]
    cnt = 0
    _dTheta = 2.0 * math.pi / collums
    for _i in range(nRings):
        #        print((_i*500.)+250.,-((_i*500.)+250.))
        for _j in range(collums):
            _theta,_z = _j*_dTheta,(_i*spacing)+delta
            
            x.append(radius * math.cos(_theta))
            y.append(radius * math.sin(_theta))
            z.append(_z)
            dx.append(-math.cos(_theta)*inv)
            dy.append(-math.sin(_theta)*inv)
            dz.append(0.0)
            type.append(_type)
            x.append(radius * math.cos(_theta))
            y.append(radius * math.sin(_theta))
            z.append(-_z)
            dx.append(-math.cos(_theta)*inv)
            dy.append(-math.sin(_theta)*inv)
            dz.append(0.0)
            type.append(_type)
            cnt+=2


#    print(cnt)
    return x,y,z,dx,dy,dz,type,cnt




x_t,y_t,z_t,dx_t,dy_t,dz_t,type_t,cnt_t = topcap(6700.,13,13,1)
x_b,y_b,z_b,dx_b,dy_b,dz_b,type_b,cnt_b = bottomcap(6700.,13,13,1)
x_s,y_s,z_s,dx_s,dy_s,dz_s,type_s,cnt_s = sidePMTs(6700.,84,13,1)

x_tv,y_tv,z_tv,dx_tv,dy_tv,dz_tv,type_tv,cnt_tv = topcap(   7200.,5,5,2,-1, 0,spacing = 1370.,delta = 685.)
x_bv,y_bv,z_bv,dx_bv,dy_bv,dz_bv,type_bv,cnt_bv = bottomcap(7200.,5,5,2,-1,   spacing = 1370.,delta = 685.)
x_sv,y_sv,z_sv,dx_sv,dy_sv,dz_sv,type_sv,cnt_sv = sidePMTs( 7200.,14,5,2,-1,  spacing = 1370.,delta = 685.)



print("////",cnt_t,cnt_b,cnt_s)
print("{")
print("//// Total number of inner PMTs : ",cnt_t+cnt_s+cnt_b)
print("//// Total number of veto PMTs : ",cnt_tv+cnt_sv+cnt_bv)
print("\"name\": \"PMTINFO\",")
print("\"valid_begin\": [0, 0],")
print("\"valid_end\": [0, 0],")
print("\"x\":"    ,x_t   + x_b    + x_s + x_tv   + x_bv    + x_sv,",")
print("\"y\":"    ,y_t   + y_b    + y_s + y_tv   + y_bv    + y_sv,",")
print("\"z\":"    ,z_t   + z_b    + z_s + z_tv   + z_bv    + z_sv,",")
print("\"dir_x\":",dx_t  + dx_b   + dx_s + dx_tv  + dx_bv   + dx_sv,",")
print("\"dir_y\":",dy_t  + dy_b   + dy_s + dy_tv  + dy_bv  + dy_sv,",")
print("\"dir_z\":",dz_t  + dz_b   + dz_s + dz_tv  + dz_bv   + dz_sv,",")
print("\"type\":",type_t + type_b + type_s + type_tv + type_bv + type_sv,",")
print("}\n\n")

#print("////",cnt_tv,cnt_bv,cnt_sv)
#print("{")
#print("\"name\": \"PMTINFO\",")
#print("\"valid_begin\": [0, 0],")
#print("\"valid_end\": [0, 0],")
#print("\"x\":"    ,x_tv   + x_bv    + x_sv,",")
#print("\"y\":"    ,y_tv   + y_bv    + y_sv,",")
#print("\"z\":"    ,z_tv   + z_bv    + z_sv,",")
#print("\"dir_x\":",dx_tv  + dx_bv   + dx_sv,",")
#print("\"dir_y\":",dy_tv  + dy_bv  + dy_sv,",")
#print("\"dir_z\":",dz_tv  + dz_bv   + dz_sv,",")
#print("\"type\":",type_tv + type_bv + type_sv,",")
#print("}\n\n")

print("//// Total number of PMTs : ",cnt_tv+cnt_sv+cnt_bv+cnt_t+cnt_s+cnt_b)


#
#
