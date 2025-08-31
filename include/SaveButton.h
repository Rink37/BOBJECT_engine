#ifndef SAVEBUTTONDEF
#define SAVEBUTTONDEF
#include "ImageDataType.h"

extern const int SaveButtonWidth;
extern const int SaveButtonHeight;
extern const int SaveButtonChannels;
extern unsigned char SaveButtonBytes[];
#define SAVEBUTTON imageData(SaveButtonWidth, SaveButtonHeight, SaveButtonChannels, SaveButtonBytes )

#endif