#ifndef TESTCHECKBOXBUTTONDEF
#define TESTCHECKBOXBUTTONDEF
#include "ImageDataType.h"

extern const int TestCheckboxButtonWidth;
extern const int TestCheckboxButtonHeight;
extern const int TestCheckboxButtonChannels;
extern unsigned char TestCheckboxButtonBytes[];
#define TESTCHECKBOXBUTTON imageData(TestCheckboxButtonWidth, TestCheckboxButtonHeight, TestCheckboxButtonChannels, TestCheckboxButtonBytes )

#endif