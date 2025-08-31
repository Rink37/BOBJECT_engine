#ifndef WEBCAMOFFBUTTONDEF
#define WEBCAMOFFBUTTONDEF
#include "ImageDataType.h"

extern const int WebcamOffButtonWidth;
extern const int WebcamOffButtonHeight;
extern const int WebcamOffButtonChannels;
extern unsigned char WebcamOffButtonBytes[];
#define WEBCAMOFFBUTTON imageData(WebcamOffButtonWidth, WebcamOffButtonHeight, WebcamOffButtonChannels, WebcamOffButtonBytes )

#endif