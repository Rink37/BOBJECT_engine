#ifndef SETTINGSBUTTONDEF
#define SETTINGSBUTTONDEF
#include "ImageDataType.h"

extern const int SettingsButtonWidth;
extern const int SettingsButtonHeight;
extern const int SettingsButtonChannels;
extern unsigned char SettingsButtonBytes[];
#define SETTINGSBUTTON imageData(SettingsButtonWidth, SettingsButtonHeight, SettingsButtonChannels, SettingsButtonBytes )

#endif