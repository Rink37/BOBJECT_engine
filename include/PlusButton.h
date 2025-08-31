#ifndef PLUSBUTTONDEF
#define PLUSBUTTONDEF
#include "ImageDataType.h"

extern const int PlusButtonWidth;
extern const int PlusButtonHeight;
extern const int PlusButtonChannels;
extern unsigned char PlusButtonBytes[];
#define PLUSBUTTON imageData(PlusButtonWidth, PlusButtonHeight, PlusButtonChannels, PlusButtonBytes )

#endif