#ifndef NORMALTEXTDEF
#define NORMALTEXTDEF
#include "ImageDataType.h"

extern const int NormalTextWidth;
extern const int NormalTextHeight;
extern const int NormalTextChannels;
extern unsigned char NormalTextBytes[];
#define NORMALTEXT imageData(NormalTextWidth, NormalTextHeight, NormalTextChannels, NormalTextBytes )

#endif