#ifndef OSBUTTONDEF
#define OSBUTTONDEF
#include "ImageDataType.h"

extern const int OSButtonWidth;
extern const int OSButtonHeight;
extern const int OSButtonChannels;
extern unsigned char OSButtonBytes[];
#define OSBUTTON imageData(OSButtonWidth, OSButtonHeight, OSButtonChannels, OSButtonBytes )

#endif