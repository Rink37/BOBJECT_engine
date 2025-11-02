# MOBJECT Engine

This project is designed to help me make traditionally hand-painted textures for my 3D models in a single contained solution, but I am also trying to make it accessible so that other people who are interested can do the same!

## Usage

On launch the app will detect the default webcam used by the system. Point the camera at a specific canvas with corners marked by a single distinctive colour, then drag the sliders until only those corners are white. If set up properly, you should see your webcam view cropped to only the canvas to the right. If there's an issue you can hit the settings button at the top. To pause/play the webcam view, hit the pause button in the top middle.

The app enables you to paint diffuse textures and normal maps using traditional methods. By default the webcam is the diffuse texture, but if you hit the 'plus' button next to the normal label you will see another image view appear which describes the object-space map of the last loaded model. If you hit the webcam button next to this you can toggle using the webcam view as the active normal! When painting the normal you will see how the painted normal affects the lighting of your object, but if you wish to see the normal as a diffuse you can switch to the camera view by clicking the leftmost button under the 'load object' button. If the model looks unshadowed, toggle shadows using the 'lit' button at the top. 

The app also contains functions for saving and loading each of the map types so you can use them in other projects. If the webcam view is enabled, the save function will capture a screenshot of the current cropped frame, but loading an image while in the image view will not overwrite the webcam view - it will just disable the webcam view. The webcam view and loaded image can be switched between using the camera toggle next to each map name. 

The normal map component also has the ability to generate maps from the object. The object-space map of the mesh normals is generated automatically, but any OS map can be converted to a tangent-space map by clicking the button marked 'OS' so that it changes to 'TS'. This function is currently not invertible (though it will be soon), so if you load a tangent space map while the icon is 'TS' you can't convert it to an object space map - it will just load the last default. 

I'm working on functionality to remap a normal map using a diffuse texture but this isn't fully implemented yet, so I've disabled it in the release build.

## FAQ

  * Operating Systems?
    - Tested only on Windows 11

  * Can the webcam device be changed?
    - No, not yet - though this is something I aim to add soon.
