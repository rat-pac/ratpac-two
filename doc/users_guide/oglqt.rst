Visualization with OpenGL-Qt
----------------------------
Visualizations using opengl allow graphics to be rendered and adjusted real-time
in a relatively performant way. One can visualize the whole detector or slices
of the detector with particle tracks to debug / demonstrate the detector.

Setting up GEANT4
`````````````````
In your mac directory you should create a file called vis.mac. This file will
hold all of your visualization information. Here is an example::

    /glg4debug/glg4param omit_muon_processes  1.0
    /glg4debug/glg4param omit_hadronic_processes  1.0
    
    /run/initialize
    
    /vis/open OGLSQt
    /vis/scene/create
    /vis/scene/add/trajectories #additionally can add rich and/or smooth
    /tracking/storeTrajectory 1
    /tracking/FillPointCont 1
    /vis/scene/add/volume
    /vis/scene/add/hits
    /vis/sceneHandler/attach

    /vis/viewer/set/upVector 0.0 0.0 1.0
    /vis/viewer/set/viewpointThetaPhi 90 180
    /vis/viewer/zoomTo 20
    /vis/viewer/set/style s
    
    ## Cut a plane through the detector
    /vis/viewer/addCutawayPlane -100 0 0 cm 1 0 0

Running rat
```````````
In order to keep rat from exiting the moment the macro completes, place rat
into interactive mode. This can either be done standalone::

    rat --vis

Or even in combination with a list of macros::
    rat vis.mac --vis
