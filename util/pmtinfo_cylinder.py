'''
Create a PMTINFO RATDB table with positions of PMTs arranged on a
cylinder. This is the same as baseline.py but wrapped in argparse.
'''

import numpy as np
import math
import argparse

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", "--outfile", default="PMTINFO.ratdb")
    parser.add_argument("-c", "--coverage", type=float, default=0.20)
    parser.add_argument("--veto_coverage",  type=float, default=0.02)
    parser.add_argument("-r", "--radius",   type=float, default=6700)
    parser.add_argument("--pmt_radius",     type=float, default=126.5)
    parser.add_argument("--circletop", action='store_true')
    return parser.parse_args()

def topcap(height,rangeX,rangeY,_type,inv = 1.0, inner = 1.0,
        spacing = 500.,delta = 250.,tolerance = 200., circletop=False):
    x,y = [],[]
    cnt = 0
    delta = spacing/2.0
    for _i in range(rangeX):
        for _j in range(rangeY):
            _x,_y = (_i*spacing + delta),(_j*spacing + delta)
            if np.sqrt(_x*_x+_y*_y)< height - tolerance:  #Asummes right cylinder such that Height = radius
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

    return x,y,z,dx,dy,dz,type,cnt

def bottomcap(height,rangeX,rangeY,_type,inv = 1.0,spacing = 500.,delta = 250.,
        tolerance = 200., circletop=False):
    delta = spacing/2.0
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

    return x,y,z,dx,dy,dz,type,cnt



def sidePMTs(radius,collumns,nRings,_type,inv = 1.0,spacing = 500.,delta = 250.):
    delta = spacing / 2.0 
    x,y,z,dx,dy,dz,type = [],[],[],[],[],[],[]
    cnt = 0
    _dTheta = 2.0 * math.pi / collumns
    for _i in range(nRings):
        #        print((_i*500.)+250.,-((_i*500.)+250.))
        for _j in range(collumns):
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

    return x,y,z,dx,dy,dz,type,cnt

def cap_range_xy(radius, coverage, pmt_radius):
    face        = np.pi * pmt_radius**2
    area        = np.pi * radius**2
    approx_pmts = coverage*area/face
    print(f'Face: {face}')
    return approx_pmts

def main():
    args = get_args()
    with open(args.outfile, "w") as f:
        ## Lets try to copy the geo factory here
        photocathode_coverage = args.coverage
        veto_coverage         = args.veto_coverage
        veto_offset           = 700
        photocathode_radius   = args.pmt_radius
        photocathode_area     = np.pi * photocathode_radius**2
        pmt_radius            = args.radius
        veto_radius           = pmt_radius + veto_offset
        surface_area          = 6*np.pi*pmt_radius**2
        veto_surface_area     = 6*np.pi*veto_radius**2

        required_pmts         = np.ceil( photocathode_coverage * surface_area / photocathode_area )
        required_veto_pmts    = np.ceil( veto_coverage * veto_surface_area / photocathode_area )
        pmt_space             = (surface_area / required_pmts)**0.5
        veto_space            = (veto_surface_area / required_veto_pmts)**0.5

        cols                  = int( np.round(2*np.pi*pmt_radius/pmt_space) )
        rows                  = int( np.round(pmt_radius/pmt_space) )
        veto_cols             = int( np.round(2*np.pi*veto_radius/veto_space) )
        veto_rows             = int( np.round(veto_radius/veto_space) )
        rdim                  = int( np.round(pmt_radius / pmt_space) )

        print(cols, rows, rdim)

        ## Inner pmts
        print(f'Creating {rows} rows and {cols} cols for sides with a {rdim} grid on top/bot')
        print(f'{pmt_space}')
        print(f'{pmt_radius}')
        x_t,y_t,z_t,dx_t,dy_t,dz_t,type_t,cnt_t = topcap(pmt_radius,rdim,rdim,1, spacing=pmt_space)
        x_b,y_b,z_b,dx_b,dy_b,dz_b,type_b,cnt_b = bottomcap(pmt_radius,rdim,rdim,1, spacing=pmt_space)
        x_s,y_s,z_s,dx_s,dy_s,dz_s,type_s,cnt_s = sidePMTs(pmt_radius,cols,rows,1, spacing=pmt_space)
        
        x_tv,y_tv,z_tv,dx_tv,dy_tv,dz_tv,type_tv,cnt_tv = topcap(    veto_radius,veto_rows,veto_rows,2,-1, 0,spacing = veto_space, delta = 685.)
        x_bv,y_bv,z_bv,dx_bv,dy_bv,dz_bv,type_bv,cnt_bv = bottomcap( veto_radius,veto_rows,veto_rows,2,-1,   spacing = veto_space, delta = 685.)
        x_sv,y_sv,z_sv,dx_sv,dy_sv,dz_sv,type_sv,cnt_sv = sidePMTs(  veto_radius,veto_cols,veto_rows,2,-1,  spacing = veto_space, delta = 685.)

        lines_to_write = []

        print(f'Total Inner PMTS: {cnt_t+cnt_s+cnt_b}')
        print(f'Total Veto PMTS: {cnt_tv+cnt_sv+cnt_bv}')

        true_coverage = (cnt_t + cnt_s + cnt_b) * photocathode_area / surface_area
        print(f'True Photocoverage: {true_coverage:0.4f}')
        
        lines_to_write.append(f"////{cnt_t}, {cnt_b}, {cnt_s}")
        lines_to_write.append(f"{{")
        lines_to_write.append(f"//// Total number of inner PMTs: {cnt_t+cnt_s+cnt_b}")
        lines_to_write.append(f"//// Total number of veto PMTs: {cnt_tv+cnt_sv+cnt_bv}")
        lines_to_write.append(f"\"name\": \"PMTINFO\",")
        lines_to_write.append(f"\"valid_begin\": [0, 0],")
        lines_to_write.append(f"\"valid_end\": [0, 0],")
        lines_to_write.append(f"\"x\": {x_t + x_b + x_s + x_tv + x_bv + x_sv},")
        lines_to_write.append(f"\"y\": {y_t + y_b + y_s + y_tv + y_bv + y_sv},")
        lines_to_write.append(f"\"z\": {z_t + z_b + z_s + z_tv + z_bv + z_sv},")
        lines_to_write.append(f"\"dir_x\": {dx_t + dx_b + dx_s + dx_tv + dx_bv + dx_sv},")
        lines_to_write.append(f"\"dir_y\": {dy_t + dy_b + dy_s + dy_tv + dy_bv + dy_sv},")
        lines_to_write.append(f"\"dir_z\": {dz_t + dz_b + dz_s + dz_tv + dz_bv + dz_sv},")
        lines_to_write.append(f"\"type\": {type_t + type_b + type_s + type_tv + type_bv + type_sv},")
        lines_to_write.append(f"}}\n\n")

        output = '\n'.join(lines_to_write)
        f.write(output)
        
        print("//// Total number of PMTs : ",cnt_tv+cnt_sv+cnt_bv+cnt_t+cnt_s+cnt_b)

if __name__ == '__main__':
    main()
