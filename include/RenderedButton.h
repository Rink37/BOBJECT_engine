#ifndef RENDEREDBUTTONDEF
#define RENDEREDBUTTONDEF
#include "ImageDataType.h"

extern const int RenderedButtonWidth;
extern const int RenderedButtonHeight;
extern const int RenderedButtonChannels;
extern unsigned char RenderedButtonBytes[];
#define RENDEREDBUTTON imageData(RenderedButtonWidth, RenderedButtonHeight, RenderedButtonChannels, RenderedButtonBytes )

#endif