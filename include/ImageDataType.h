#ifndef IMAGEDATA
#define IMAGEDATA

struct imageData{
    const int Width;
    const int Height;
    const int Channels;
    const unsigned char *Bytes;
};

#endif