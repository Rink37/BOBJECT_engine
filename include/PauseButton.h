#ifndef PAUSEBUTTONDEF
#define PAUSEBUTTONDEF
#include "ImageDataType.h"

extern const int PauseButtonWidth;
extern const int PauseButtonHeight;
extern const int PauseButtonChannels;
extern unsigned char PauseButtonBytes[];
#define PAUSEBUTTON imageData(PauseButtonWidth, PauseButtonHeight, PauseButtonChannels, PauseButtonBytes )

#endif