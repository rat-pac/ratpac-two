import argparse
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", default="PMTINFO.ratdb",
            help="PMTInfo file")
    parser.add_argument("--style", default="quiver")
    return parser.parse_args()

class pmtinfo:
    def __init__(self, filename):
        with open(filename) as fo:
            lines = [line for line in fo if ':' in line]
            lines = [line.replace('\n', '').split(':') for line in lines]
            self.data = {l[0]: l[1] for l in lines}
        self.x = np.array(eval(self.data['"x"'])[0])
        self.y = np.array(eval(self.data['"y"'])[0])
        self.z = np.array(eval(self.data['"z"'])[0])
        self.u = np.array(eval(self.data['"dir_x"'])[0])
        self.v = np.array(eval(self.data['"dir_y"'])[0])
        self.w = np.array(eval(self.data['"dir_z"'])[0])
        self.t = np.array(eval(self.data['"type"'])[0])

def main():
    args = get_args()
    data = pmtinfo(args.i)

    fig = plt.figure(figsize=(10,10))
    ax = fig.add_subplot(111, projection='3d')
    #ax.scatter( data.x, data.y, data.z )
    ## Plot Inner PMTs
    inner = (data.t == 1)
    outer = (data.t == 2)
    if args.style == "quiver":
        ax.quiver( data.x[inner], data.y[inner], data.z[inner], 
                   data.u[inner], data.v[inner], data.w[inner],
                   length=300, pivot='tip', color='xkcd:cerulean'
                 )
        ax.quiver( data.x[outer], data.y[outer], data.z[outer], 
                   data.u[outer], data.v[outer], data.w[outer],
                   length=300, pivot='tip', color='xkcd:maroon'
                 )
    else:
        ax.scatter( data.x[inner], data.y[inner], data.z[inner], 
                    color='xkcd:cerulean'
                  )
        ax.scatter( data.x[outer], data.y[outer], data.z[outer], 
                    color='xkcd:maroon'
                  )

    plt.savefig("pmtinfo.pdf")
    plt.show()

if __name__ == '__main__':
    main()
