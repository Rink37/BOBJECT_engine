# BOBJECT Engine

An engine to render traditionally painted textures onto 3D meshes in real time, as an all-in-one contained solution.

> [!CAUTION]
> This project is still in development and not fully accessible for a majority of its potential user group. Use with caution / contribute to allow for a faster progression into a more usable state

## Installation
> [!NOTE]
> The engine is currently missing support for other operating systems and is therefore only available for windows. You can observe the progress on linux compatibility [here](https://github.com/Rink37/BOBJECT_engine/pull/2)

### Windows:
Download the latest release from the release tab.

## Usage
You can always pause the webcam view using the pause button in the top middle of the engine

### Camera Setup
1. Mark your canvasses corners with a single distinctive color
2. Point your camera onto your canvas

### Setup
1. Launch the engine, it will detect the default webcam of your device
2. Drag the sliders until only the marked corners are white

### Textures
The app enables you to paint diffuse textures and normal maps using traditional methods.

By default, the webcam is the diffuse texture. You can hit the 'plus' button next to the normal label to see the image view of the object-space map from the last-loaded model.

If you hit the webcam button next to the 'plus' button, you can toggle using the webcam view as the active normal.

When painting the normal, you will see how the painted normal affects the lighting of your object. If you wish to see the normal as a diffuse, you can switch to the camera view by clicking the leftmost button under the 'load object' button. If the model looks unshadowed, toggle shadows using the 'lit' button at the top.

### Saving and Loading
The app contains functions for saving and loading each of the map types to export them for usage in other projects.

If the webcam view is enabled, the save function will capture a screenshot of the current cropped frame. But loading an image while in the image view will not overwrite the webcam view, it will just disable the webcam view.

The webcam view and loaded image can be switched between using the camera toggle next to each map name.

### Generate Maps
The normal map component can also generate maps from the object. The object-space map of the mesh normals is generated automatically, but any OS map can be converted to a tangent-space map by clicking the button marked 'OS' so that it changes to 'TS'.

> [!Note]
> This function is currently not invertible (this will change soon). So if you load a tangent space map while the icon is 'TS' you can't convert it to an object space map. It will just load the last default.

## FAQ

**Can the webcam device be changed?** — No, not yet, though this is something I aim to add soon.

## TODO
- [ ] I'm working on a function to remap a normal map using a diffuse texture, but this isn't fully implemented yet, so I've disabled it in the release build.
- [ ] Converting a tangent space map while the 'TS' icon is active to an object space map
- [ ] Changeable webcam devices
