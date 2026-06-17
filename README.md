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

## Image Creation Tools

The application also features a few different ways of modifying images to help you create 3D models rendered in more interesting ways. All image modification operations are called from the texture settings menu which is accessed by clicking on the settings button next to the label for each texture in the texture menu. This will bring up a menu which lets you view the texture and which contains a dropdown menu prompting you to select a function, each of which is an image processing operation which can be performed. The operations available to each texture may differ based on the type of image that they are. 

All texture modification processes will automatically update textures and any materials which use them, so it is recommended that you apply textures to objects via materials before modifying them so that you can easily tell what effect the filtering algorithms are having on the appearance of the object. 

### Remap Images to Match Brushstrokes
The app contains three different algorithms which can be used to 'remap' an arbitrary image so that the colours are constant across the area of each brushstroke. These algorithms were designed to be used on normal maps so that the user could produce normal maps which are flattened across the brush area, making the normal map look hand-painted as well as the diffuse. However, the algorithms are generic - you can use them as you wish! 

Using the remapper will first open a menu prompting you to select a reference texture: this is the texture containing the brushstrokes which you will attempt to match the structure of the current texture to. Once a texture has been selected, clicking the finish button will bring up the main remapper menu. In this menu, the most important components are the three buttons beneath the image viewer. The arrow buttons allow you to cycle through the different algorithms, and the button in the middle toggles whether the remapped image should be normalized or not.

A brief description of each of the three algorithms is as follows (starting on the default and listing in the order you would see by clicking the right button repeatedly):
1. The "Iterative" algorithm: this algorithm averages only small pixel groups based on colour similarity in the references, but it will repeat this averaging many times over to spread averaged values further. PROS: The results can be very high quality, CONS: High quality results can take a while to process properly.  
2. The "Coord mapped" algorithm: this algorithm functions similarly to the iterative algorithm, but instead of averaging colours in your texture we average image positions to find the central coordinate of each brushstroke and set each pixel to be the centroid value. Then in the final step we read the value from the texture at each centroid, and paste that value over all pixels with the same centroid value. PROS: This system isn't prone to averaging errors - e.g remapping a height map will not result in midpoint heights if not wanted, CONS: The colours are not averaged, so the algorithm is highly prone to random noise errors. 
3. The "Kuwahara-based" algorithm: this algorithm uses the kuwahara filter to average pixel colours across brushstrokes very effectively and produce clear borders between remapped brushstroke areas. PROS: Very flat, consistent colours across brushstrokes, CONS: Resolution limit of 1024px, lack of fine detail in borders between brushstrokes. 

Each of these algorithms have multiple parameters which you can modify using the sliders in the menu. Describing the effect of each parameter on each algorithm will make this description become far too large, so I won't go into any detail here. In the future I will probably create better documentation to describe exactly what each does, but for now you'll have to figure out what each of them do by playing with the system for yourself!

### Fix the Seams in a Texture

This is the newest function provided by the application, giving users the ability to fix seams in their hand painted textures. In this context a seam refers to an area where two unconnected parts of the UV map join in the 3D model. This appears on your models as sudden sharp jumps between two different areas of the painted map, which doesn't match with the aesthetic of the rest of the piece. The seam fixer aims to address this by copying image data from one side of the seam to the other so that the texture appears to continue across the seam rather than jumping suddenly. 

Because this function is entirely based on the mesh settings, clicking on the seam fixing function will open a menu prompting you to select the mesh you want to fix seams with. Once a mesh is selected, clicking finish will bring you to the main menu for the seam fixer. Since this algorithm is fairly new, there are limited options for user-control. Currently, this menu features a toggle button which lets you switch which side of each seam is copied from and which side is copied to. There is no support for modifying individual seams currently, though this functionality is planned for the future. The seam fixer menu also features a slider which lets you specify the blending distance - a larger distance means that more image data will be copied from one side to another, making the border smoother. 

> [!WARNING]
> The current seam fixer implementation assumes that the painted map extends beyond the edges of the UV islands. If this is not the case, the seam fixer will not work - rather, it will make the result worse. I plan to produce alternate options in the future which address this assumption. 

### Generate and Modify Normal Maps

The application features support for generating object-space maps directly from a loaded mesh. To create an object-space map from a mesh, open the object settings menu using the settings icon next to the model label then click the plus button at the bottom right of the window. This will create an object-space map labelled as {object name}_OSNorm, which will be added into the texture menu. 

Normal maps also have access to a few normal-specific image processing options: map space transitioning and map mixing. Both of these options are accessed the same as any other image processing operation, but will only appear if a texture has been correctly configured as a normal. 

The map space transitioning option is used to change the space of a normal map from OS to TS or TS to OS. This operation requires you to select the object which the maps are specific to, since the mesh normals are used to guide the transition. You will have the option to specify whether this operation changes the texture that you use the operation on, or if the system will create a new texture to be added to the texture menu. This is configured by toggling the button next to the "In place?" label in the operation menu. If the icon is a solid circle, the operation will overwrite the texture, but if the icon is empty the system will create a new texture named {Normal map name}_{New type} (e.g. a normal map named "Normal" which is OS by default will be used to create a texture labelled "Normal_TS").

Normal map mixing is what it sounds like - combining the effects of two normal maps into a single normal (e.g. if you had one normal map which created a design on a surface, and another which added smaller details like scratches or dirt you could mix them to produce a dirty/scratched design). Calling this operation on a normal map will open a menu asking you to specify which object your normals are relative to. This mesh is necessary because the normal map space transition operation needs to be called during the mixing process. The menu will also ask you to specify what other normal map you want to mix your normal with. Once both have been selected, click the "Finish" button to replace the original normal with the mixed one.

> [!Warning]
> The normal mixer will modify the original texture which the operation is called on. If you want to save the normal before it is mixed as well as after, save the image before calling the normal mixing functionality. 

### Extract physical canvas surface details
The application contains basic 3D scanning functionality which is designed to calculate diffuse and normal maps that describe the physical canvas surface using multiple photos of the canvas with the light source in multiple different locations. This functionality can be used to produce diffuse maps with no shadows produced by surface details or to create normals that digitally reproduce the lighting effects of the painting surface details.

This functionality can be accessed by pressing the settings icon next to the label "Extract from painting" at the top of the texture list in the texture menu. Clicking on this button will produce a menu asking you to select a reference texture. This texture is used as a template reference which all other loaded images are matched to, so it needs to be a complete picture of the painting which is correctly aligned with the UV map. Once this texture has been selected, click the finish button to reach the next step of the process.

When the tomography menu is open, individual scans of the painting with different light angles can be loaded (which don't need to contain the entire canvas, since the matching functionality works with an incomplete image). When an image has been loaded, you'll need to drag the button which appears over the image until it matches the position of the light relative to the image. The slider to the right is used to set the angle of the light above the surface of the canvas. Hitting the 'update' button will then correct the loaded image so that it matches the shape of the reference diffuse (and will rotate the light position slider by the same amount). Then hitting 'finish' will add the image to the set of images used in the map generation. 

Once a reasonable set of images has been loaded, use the checkboxes next to the image type labels to specify what maps should be generated, then hit 'update' to perform map generation. Once this process is finished, the material applied to the mesh/plane in the tomography menu will be changed to use the newly generated maps. When you are happy with the generated maps, hit 'finish' and the menu will close and the maps will be applied to the main surface menu.

>[!Note]
> Matching images to the template is not always successful, and won't work for some images. The matching algorithm will produce the same result each time so the problem can't be solved by attempting the same thing again - simply remove these images from the set either by cancelling them in the load stage or by deleting them from the image table by hitting the 'X' icon.

## TODO
- [ ] Add automatic light source detection in the tomography menu to avoid manually setting light direction
- [ ] Add alternate seam fixer options
- [ ] Create better documentation of remapper algorithm parameters
- [ ] Streamline the material creation process
- [ ] Add functionality for saving/loading of sessions
