#ifndef TANGENTSPACEDEF
#define TANGENTSPACEDEF
#include "ImageDataType.h"

extern const int TangentSpaceWidth;
extern const int TangentSpaceHeight;
extern const int TangentSpaceChannels;
extern unsigned char TangentSpaceBytes[];
#define TANGENTSPACE imageData(TangentSpaceWidth, TangentSpaceHeight, TangentSpaceChannels, TangentSpaceBytes )

#endif