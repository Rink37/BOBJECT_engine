#ifndef WEBCAMONBUTTONDEF
#define WEBCAMONBUTTONDEF
#include "ImageDataType.h"

extern const int WebcamOnButtonWidth;
extern const int WebcamOnButtonHeight;
extern const int WebcamOnButtonChannels;
extern unsigned char WebcamOnButtonBytes[];
#define WEBCAMONBUTTON imageData(WebcamOnButtonWidth, WebcamOnButtonHeight, WebcamOnButtonChannels, WebcamOnButtonBytes )

#endif