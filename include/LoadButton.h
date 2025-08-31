#ifndef LOADBUTTONDEF
#define LOADBUTTONDEF
#include "ImageDataType.h"

extern const int LoadButtonWidth;
extern const int LoadButtonHeight;
extern const int LoadButtonChannels;
extern unsigned char LoadButtonBytes[];
#define LOADBUTTON imageData(LoadButtonWidth, LoadButtonHeight, LoadButtonChannels, LoadButtonBytes )

#endif