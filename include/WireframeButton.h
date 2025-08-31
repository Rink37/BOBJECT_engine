#ifndef WIREFRAMEBUTTONDEF
#define WIREFRAMEBUTTONDEF
#include "ImageDataType.h"

extern const int WireframeButtonWidth;
extern const int WireframeButtonHeight;
extern const int WireframeButtonChannels;
extern unsigned char WireframeButtonBytes[];
#define WIREFRAMEBUTTON imageData(WireframeButtonWidth, WireframeButtonHeight, WireframeButtonChannels, WireframeButtonBytes )

#endif