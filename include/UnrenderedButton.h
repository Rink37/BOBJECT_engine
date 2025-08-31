#ifndef UNRENDEREDBUTTONDEF
#define UNRENDEREDBUTTONDEF
#include "ImageDataType.h"

extern const int UnrenderedButtonWidth;
extern const int UnrenderedButtonHeight;
extern const int UnrenderedButtonChannels;
extern unsigned char UnrenderedButtonBytes[];
#define UNRENDEREDBUTTON imageData(UnrenderedButtonWidth, UnrenderedButtonHeight, UnrenderedButtonChannels, UnrenderedButtonBytes )

#endif