#ifndef D2NBUTTONDEF
#define D2NBUTTONDEF
#include "ImageDataType.h"

extern const int D2NButtonWidth;
extern const int D2NButtonHeight;
extern const int D2NButtonChannels;
extern unsigned char D2NButtonBytes[];
#define D2NBUTTON imageData(D2NButtonWidth, D2NButtonHeight, D2NButtonChannels, D2NButtonBytes )

#endif