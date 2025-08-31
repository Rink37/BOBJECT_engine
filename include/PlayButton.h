#ifndef PLAYBUTTONDEF
#define PLAYBUTTONDEF
#include "ImageDataType.h"

extern const int PlayButtonWidth;
extern const int PlayButtonHeight;
extern const int PlayButtonChannels;
extern unsigned char PlayButtonBytes[];
#define PLAYBUTTON imageData(PlayButtonWidth, PlayButtonHeight, PlayButtonChannels, PlayButtonBytes )

#endif