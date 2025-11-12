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

### Generate and Translate Normal Maps
The normal map component can also generate maps from the object. The object-space (OS) map of the mesh normals is generated automatically, but any OS map can be converted to a tangent-space (TS) map by clicking the button marked 'OS' so that it changes to 'TS'. This function can also be inverted, allowing conversion from a TS map to an OS map by clicking the button again.

> [!Note]
> This function has no ability to determine what type of normal is loaded and has no option to change the type of the normal after it has been loaded. Users must set the OS/TS button to the correct state before loading a map of either type, otherwise lighting and space translation functions will not work as expected.

### Remap Normals to Match Diffuse Brushstrokes
The app contains an algorithm which can be used to 'remap' an object-space normal so that individual brushstrokes in the diffuse appear flatly lit. This process is used to imitate the effect of hand-painted normals but to ensure that the normal vectors are correct to a model and appear to match with a diffuse. To use this function, load a diffuse image for your model (the webcam view will not work) and generate an OS normal map. Then, clicking the 'Diff->Norm' button will open a new UI panel where you can use sliders to modify the parameters of the algorithm, and then hitting finish closes the remap menu and applies the map as your current OS map, which can then be saved. 

The function of the sliders is as follows:

1. Search Size: Modifies the area that is searched over when seeking brushstrokes - smaller values lead to smaller individually lit brushstrokes and vice-versa. This parameter should be adjusted until the light doesn't appear to be broken up over the surface of each brushstroke.
2. Noise Removal: Modifies the amount of detail noise (e.g. canvas texture, shadows) which is ignored in the diffuse map. Smaller values will result in more harshly broken up brushstrokes than larger values.
3. Edge Sharpness: Modifies how sharp the border between brushstrokes is. Smaller values lead to smoother transitions between brushstrokes and vice versa. 
4. Stroke Flatness: Modifies the extent to which each stroke is flattened - smaller values mean that the brushstrokes will appear more rounded and higher values make the brushstrokes appear more uniformly lit.
5. Flatten Threshold: Modifies the threshold which the system uses to separate faces that are flattened. Smaller values lead to smaller flattened areas, whereas larger values lead to flattening being performed over larger distances. However, when the value is too large this can also lead to 'bleed' where adjacent similar colours are flattened to face in the same direction as each other rather than unique stroke specific directions.


>[!Note]
> The remapping algorithm currently has a maximum diffuse height resolution limit of 1024px due to the risks of GPU timeout when using some of the more complex compute shaders used in the remap pipeline. Larger diffuse images can be loaded and will not cause issues, but the remapper will downscale them before performing any filtering and then upscale them to the original dimensions after, yielding no quality improvement from larger diffuse images. 

## FAQ

**Can the webcam device be changed?** — No, not yet, though this is something I aim to add soon.

## TODO
- [x] Remapping normals using the diffuse texture.
- [x] Converting a tangent space map while the 'TS' icon is active to an object space map
- [ ] Changeable webcam devices
- [ ] Basic tomography functionality to extract information about the physical surface of the painting to be used in rendering
