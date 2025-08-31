#ifndef OPENBUTTONDEF
#define OPENBUTTONDEF
#include "ImageDataType.h"

extern const int OpenButtonWidth;
extern const int OpenButtonHeight;
extern const int OpenButtonChannels;
extern unsigned char OpenButtonBytes[];
#define OPENBUTTON imageData(OpenButtonWidth, OpenButtonHeight, OpenButtonChannels, OpenButtonBytes )

#endif