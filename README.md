# BOBJECT Engine

An application to render traditionally painted textures onto 3D meshes in real time, as an all-in-one contained solution. The application also features methods of processing or creating images to enable a wide variation of hand-painted texture aesthetics.

> [!CAUTION]
> This project is still in development and not fully accessible for a majority of its potential user group. Use with caution / contribute to allow for a faster progression into a more usable state

## Installation
> [!NOTE]
> The engine is currently missing support for other operating systems and is therefore only available for windows. You can observe the progress on linux compatibility [here](https://github.com/Rink37/BOBJECT_engine/pull/2)

### Windows:
Download the latest release from the release tab.

## How to hand-paint your textures
The application contains a lot of features designed to make it relatively easy to setup painting the textures of your models with physical methods. Here is a quick-start guide!

### Before launching the app
1. Mark all the corners of your canvas with one specific colour (e.g. colour in the corners using a marker)
2. Point your webcam at your canvas so that all four coloured corners are visible

### Once you've set up your webcam
1. Launch the application and it will find all available webcams. By default the app will select the default webcam of your system, but you will have the option to change this in the next step.
2. Load an object by clicking on the open button next to the "meshes" label at the left of the interface. You should see a screenshot of your webcam view wrapped around the object. 
> [!NOTE]
> At the moment, the app only supports loading .obj models, though further compatibility with other model formats is planned for the future.
3. Click on the settings icon in the top middle of the app. This will open a menu which allows you to configure the settings of your webcam. You should see your webcam start updating automatically. If the wrong webcam is selected, hit the arrow keys until the viewer shows the feed from the webcam pointed at your canvas. 
4. Click on the settings button at the bottom left of this menu. It will open a window which looks all-white except for some sliders at the top. These sliders are used to specify the colour of the corners of your canvas - start dragging them around and you will see parts of the white space turn black as their colours are filtered out. Drag these sliders until all of the image is black except the corners, which should all be white. Hit the exit button in the corner of this menu.
5. Another window will open showing you the webcam view, but you can safely exit this window as well.
6. The viewer in the webcam settings menu should now show only the canvas area - everything else should have been cropped out. If this is not what you see, hit the settings button again and repeat the previous process, making sure that all four corners are fully visible and nothing else is. 
7. A few other options are provided for modifying the viewer: the slider at the top will modify the aspect ratio of your viewer, though this can be reset using the button to the right of the slider. You can also rotate the viewer to the left or right using the rotation buttons.
8. Once you are happy with how the webcam viewer looks, hit the 'Finish' button. The settings menu will close, and you should see your canvas wrapped around the model.
9. To help you paint, you can either click the settings button next to the "Webcam View" label in the "Textures" menu or you can click the webcam view mode button in the top-left of the window. Both will bring up a webcam image viewer, though the webcam view mode button will also hide much of the UI to make it easier to focus on the painting. 
10. It's time to make your masterpiece!

> [!Caution]
> Currently you will not be able to save the settings of your webcam viewer, so you will need to repeat this process every time you re-open the app. I aim to add functionality to save your settings in the future, but for now I hope you only need one session!

## View Settings

The application features multiple view modes: the webcam view mode which was described earlier, a "Rendered" view mode which is toggled using the center icon in the trio at the top-left and a "Wireframe" view mode toggled using the leftmost of these icons. 

The rendered view mode will allow the user to see the object with whatever material is applied to it and with lighting enabled. While in the rendered view mode the position of the scene light can be modified using the two sliders beneath the trio of view mode toggles. Lighting can also be disabled in the rendered view mode by clicking on the circle icon next to the settings button at the top middle of the application window. Clicking on this icon again will re-enable lighting.

The wireframe view mode will show only the wireframe of your models, so no lighting or materials are present in this mode. 

## Textures and Materials

### Saving and Loading Textures

When not in the webcam view mode you will see a menu with a "TEXTURES" label at the bottom-left of the application window. Clicking the button to the right of this label will allow you to load images into the application. While loading an image you will have the option to specify whether the image is a "colour" image or a "normalized vector" image via a dropdown menu. These options will let you specify if you are loading a normal map or a standard colour map. 

When loading normal maps, you will see an icon appear marked "OS" - clicking on this icon will switch it to say "TS". This toggle is used to specify whether the application interprets this image as an object-space (OS) normal map or a tangent-space (TS) normal map. This distinction will matter if you attempt to use any of the normal map processing options available later, but won't affect how the application renders these textures. 

All textures loaded into the textures menu will have two icons next to them, one of which is a "save" icon. Clicking this icon will enable you to save the labelled image to disk; this image should reflect the most recent variation of that texture in the case that the texture has been modified in the application. Using the save functionality on the webcam view texture will capture a screenshot of the current frame of the webcam viewer.

### Modifying the Materials of Your Objects

The application allows you to specify how to render each of the objects in the viewer through a (basic) materials system. Clicking on the settings button next to any of the objects in the objects menu will open up a window which is used to configure the material for that object. For now we only want to focus on the "plus" icon in the top-right of the window; this is the button which lets you create a new material for the object. 

After clicking the plus icon you will see a window with the label "Material0" at the top, and a dropdown menu prompting you to select a material. The "Material0" label text describes the name of the created material. This text can be modified: click on the label and start typing, then hit enter when you're done. Then click on the icon to the right of the "Select label" text - this will open a dropdown menu listing all of the currently available materials. 

> [!NOTE]
> The materials built into the application are primarily meant to allow the user to see how their texture modifications look on the model, so are poorly labelled. I plan to streamline the material options in the future by introducing structures which automatically choose the material from a set of similar options based on the arguments. 

Material options are labelled following a fairly basic convention:
1. Materials containing "AC" are marked as alpha-clipped materials; using textures with alpha channels will result in the model being rendered with dithered transparency. Materials omitting "AC" will have no support for transparency. 
2. Materials containing "BF" will be rendered using the Blinn-Phong lighting model (BF was chosen instead of BP due to the sound of "Phong") when lighting is enabled in the viewer settings. Any material marked as "Flat" will be unlit regardless of the view settings.
3. Materials containing either "OS" or "TS" will use a texture for the normal vectors of the model, whereas materials without will use the default normals of the mesh. As expected, "OS"-type materials will interpret the normal map as being object-space and the "TS"-type materials will interpret the normal map as being tangent-space. 

Clicking on one of the material options will change the material menu so that you can specify a texture for each of the channels in the material using the labelled dropdown menus. Once a texture has been selected for each channel, you can hit the "Finish" button to finalize the material and apply it to your object. You will then return to the object settings menu.

In the middle of the object settings menu, you will see another dropdown menu, which allows you to quickly swap between each of the materials that you've created during your current session. All materials except the "Webcam" material can be modified by clicking the settings button next to the "MATERIAL:" label; this will let you change the texture assigned to each channel in the material. 

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

### Extract physical canvas surface details
The application contains basic 3D scanning functionality which is designed to calculate diffuse and normal maps that describe the physical canvas surface using multiple photos of the canvas with the light source in multiple different locations. This functionality can be used to produce diffuse maps with no shadows produced by surface details or to create normals that digitally reproduce the lighting effects of the painting surface details.

This functionality can be toggled by pressing the "T"  key while a diffuse map is loaded in the surface menu. This diffuse map is used as a template reference which all other loaded images are matched to, so it needs to be a complete picture of the scan which is correctly aligned with the UV map. When the tomography menu is open, individual scans of the painting with different light angles can be loaded (which don't need to contain the entire canvas, since the matching functionality works with an incomplete image). When an image has been loaded, you'll need to drag the button which appears over the image until it matches the position of the light relative to the image. The slider to the right is used to set the angle of the light above the surface of the canvas. Hitting the 'update' button will then correct the loaded image so that it matches the shape of the reference diffuse (and will rotate the light position slider by the same amount). Then hitting 'finish' will add the image to the set of images used in the map generation. 

Once a reasonable set of images has been loaded, use the checkboxes next to the image type labels to specify what maps should be generated, then hit 'update' to perform map generation. Once this process is finished, the material applied to the mesh/plane in the tomography menu will be changed to use the newly generated maps. When you are happy with the generated maps, hit 'finish' and the menu will close and the maps will be applied to the main surface menu.

>[!Note]
> Matching images to the template is not always successful, and won't work for some images. The matching algorithm will produce the same result each time so the problem can't be solved by attempting the same thing again - simply remove these images from the set either by cancelling them in the load stage or by deleting them from the image table by hitting the 'X' icon.

## FAQ

**Can the webcam device be changed?** — No, not yet, though this is something I aim to add soon.

## TODO
- [x] Remapping normals using the diffuse texture.
- [x] Converting a tangent space map while the 'TS' icon is active to an object space map
- [ ] Changeable webcam devices
- [x] Basic tomography functionality to extract information about the physical surface of the painting to be used in rendering
- [ ] Add automatic light source detection in the tomography menu to avoid manually setting light direction
