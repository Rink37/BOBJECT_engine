#ifndef DIFFUSETEXTDEF
#define DIFFUSETEXTDEF
#include "ImageDataType.h"

extern const int DiffuseTextWidth;
extern const int DiffuseTextHeight;
extern const int DiffuseTextChannels;
extern unsigned char DiffuseTextBytes[];
#define DIFFUSETEXT imageData(DiffuseTextWidth, DiffuseTextHeight, DiffuseTextChannels, DiffuseTextBytes )

#endif