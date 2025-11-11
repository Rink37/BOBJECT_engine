#ifndef BAKED_IMAGES
#define BAKED_IMAGES

#include "ImageDataType.h"
extern const int LoadButtonWidth;
extern const int LoadButtonHeight;
extern const int LoadButtonChannels;
extern unsigned char LoadButtonBytes[];

#define LOADBUTTON imageData(LoadButtonWidth, LoadButtonHeight, LoadButtonChannels, LoadButtonBytes )

extern const int WireframeButtonWidth;
extern const int WireframeButtonHeight;
extern const int WireframeButtonChannels;
extern unsigned char WireframeButtonBytes[];

#define WIREFRAMEBUTTON imageData(WireframeButtonWidth, WireframeButtonHeight, WireframeButtonChannels, WireframeButtonBytes )

extern const int PauseButtonWidth;
extern const int PauseButtonHeight;
extern const int PauseButtonChannels;
extern unsigned char PauseButtonBytes[];

#define PAUSEBUTTON imageData(PauseButtonWidth, PauseButtonHeight, PauseButtonChannels, PauseButtonBytes )

extern const int PlayButtonWidth;
extern const int PlayButtonHeight;
extern const int PlayButtonChannels;
extern unsigned char PlayButtonBytes[];

#define PLAYBUTTON imageData(PlayButtonWidth, PlayButtonHeight, PlayButtonChannels, PlayButtonBytes )

extern const int RenderedButtonWidth;
extern const int RenderedButtonHeight;
extern const int RenderedButtonChannels;
extern unsigned char RenderedButtonBytes[];

#define RENDEREDBUTTON imageData(RenderedButtonWidth, RenderedButtonHeight, RenderedButtonChannels, RenderedButtonBytes )

extern const int SettingsButtonWidth;
extern const int SettingsButtonHeight;
extern const int SettingsButtonChannels;
extern unsigned char SettingsButtonBytes[];

#define SETTINGSBUTTON imageData(SettingsButtonWidth, SettingsButtonHeight, SettingsButtonChannels, SettingsButtonBytes )

extern const int TestCheckboxButtonWidth;
extern const int TestCheckboxButtonHeight;
extern const int TestCheckboxButtonChannels;
extern unsigned char TestCheckboxButtonBytes[];

#define TESTCHECKBOXBUTTON imageData(TestCheckboxButtonWidth, TestCheckboxButtonHeight, TestCheckboxButtonChannels, TestCheckboxButtonBytes )

extern const int UnrenderedButtonWidth;
extern const int UnrenderedButtonHeight;
extern const int UnrenderedButtonChannels;
extern unsigned char UnrenderedButtonBytes[];

#define UNRENDEREDBUTTON imageData(UnrenderedButtonWidth, UnrenderedButtonHeight, UnrenderedButtonChannels, UnrenderedButtonBytes )

extern const int D2NButtonWidth;
extern const int D2NButtonHeight;
extern const int D2NButtonChannels;
extern unsigned char D2NButtonBytes[];

#define D2NBUTTON imageData(D2NButtonWidth, D2NButtonHeight, D2NButtonChannels, D2NButtonBytes )

extern const int DiffuseTextWidth;
extern const int DiffuseTextHeight;
extern const int DiffuseTextChannels;
extern unsigned char DiffuseTextBytes[];

#define DIFFUSETEXT imageData(DiffuseTextWidth, DiffuseTextHeight, DiffuseTextChannels, DiffuseTextBytes )

extern const int NormalTextWidth;
extern const int NormalTextHeight;
extern const int NormalTextChannels;
extern unsigned char NormalTextBytes[];

#define NORMALTEXT imageData(NormalTextWidth, NormalTextHeight, NormalTextChannels, NormalTextBytes )

extern const int OpenButtonWidth;
extern const int OpenButtonHeight;
extern const int OpenButtonChannels;
extern unsigned char OpenButtonBytes[];

#define OPENBUTTON imageData(OpenButtonWidth, OpenButtonHeight, OpenButtonChannels, OpenButtonBytes )

extern const int PlusButtonWidth;
extern const int PlusButtonHeight;
extern const int PlusButtonChannels;
extern unsigned char PlusButtonBytes[];

#define PLUSBUTTON imageData(PlusButtonWidth, PlusButtonHeight, PlusButtonChannels, PlusButtonBytes )

extern const int SaveButtonWidth;
extern const int SaveButtonHeight;
extern const int SaveButtonChannels;
extern unsigned char SaveButtonBytes[];

#define SAVEBUTTON imageData(SaveButtonWidth, SaveButtonHeight, SaveButtonChannels, SaveButtonBytes )

extern const int TangentSpaceWidth;
extern const int TangentSpaceHeight;
extern const int TangentSpaceChannels;
extern unsigned char TangentSpaceBytes[];

#define TANGENTSPACE imageData(TangentSpaceWidth, TangentSpaceHeight, TangentSpaceChannels, TangentSpaceBytes )

extern const int OSButtonWidth;
extern const int OSButtonHeight;
extern const int OSButtonChannels;
extern unsigned char OSButtonBytes[];

#define OSBUTTON imageData(OSButtonWidth, OSButtonHeight, OSButtonChannels, OSButtonBytes )

extern const int WebcamOffButtonWidth;
extern const int WebcamOffButtonHeight;
extern const int WebcamOffButtonChannels;
extern unsigned char WebcamOffButtonBytes[];

#define WEBCAMOFFBUTTON imageData(WebcamOffButtonWidth, WebcamOffButtonHeight, WebcamOffButtonChannels, WebcamOffButtonBytes )

extern const int WebcamOnButtonWidth;
extern const int WebcamOnButtonHeight;
extern const int WebcamOnButtonChannels;
extern unsigned char WebcamOnButtonBytes[];

#define WEBCAMONBUTTON imageData(WebcamOnButtonWidth, WebcamOnButtonHeight, WebcamOnButtonChannels, WebcamOnButtonBytes )

extern const int WebcamViewButtonWidth;
extern const int WebcamViewButtonHeight;
extern const int WebcamViewButtonChannels;
extern unsigned char WebcamViewButtonBytes[];

#define WEBCAMVIEWBUTTON imageData(WebcamViewButtonWidth, WebcamViewButtonHeight, WebcamViewButtonChannels, WebcamViewButtonBytes )

extern const int CancelButtonWidth;
extern const int CancelButtonHeight;
extern const int CancelButtonChannels;
extern unsigned char CancelButtonBytes[];

#define CANCELBUTTON imageData(CancelButtonWidth, CancelButtonHeight, CancelButtonChannels, CancelButtonBytes )

extern const int FinishButtonWidth;
extern const int FinishButtonHeight;
extern const int FinishButtonChannels;
extern unsigned char FinishButtonBytes[];

#define FINISHBUTTON imageData(FinishButtonWidth, FinishButtonHeight, FinishButtonChannels, FinishButtonBytes )

extern const int EdgeSharpnessTextWidth;
extern const int EdgeSharpnessTextHeight;
extern const int EdgeSharpnessTextChannels;
extern unsigned char EdgeSharpnessTextBytes[];

#define EDGESHARPNESSTEXT imageData(EdgeSharpnessTextWidth, EdgeSharpnessTextHeight, EdgeSharpnessTextChannels, EdgeSharpnessTextBytes )

extern const int SearchSizeTextWidth;
extern const int SearchSizeTextHeight;
extern const int SearchSizeTextChannels;
extern unsigned char SearchSizeTextBytes[];

#define SEARCHSIZETEXT imageData(SearchSizeTextWidth, SearchSizeTextHeight, SearchSizeTextChannels, SearchSizeTextBytes )

extern const int StrokeFlatnessTextWidth;
extern const int StrokeFlatnessTextHeight;
extern const int StrokeFlatnessTextChannels;
extern unsigned char StrokeFlatnessTextBytes[];

#define STROKEFLATNESSTEXT imageData(StrokeFlatnessTextWidth, StrokeFlatnessTextHeight, StrokeFlatnessTextChannels, StrokeFlatnessTextBytes )

#endif