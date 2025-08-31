#ifndef WEBCAMVIEWBUTTONDEF
#define WEBCAMVIEWBUTTONDEF
#include "ImageDataType.h"

extern const int WebcamViewButtonWidth;
extern const int WebcamViewButtonHeight;
extern const int WebcamViewButtonChannels;
extern unsigned char WebcamViewButtonBytes[];
#define WEBCAMVIEWBUTTON imageData(WebcamViewButtonWidth, WebcamViewButtonHeight, WebcamViewButtonChannels, WebcamViewButtonBytes )

#endif