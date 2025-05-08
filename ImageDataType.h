#ifndef IMAGEDATA
#define IMAGEDATA

struct imageData{
    imageData(const int w, const int h, const int c, unsigned char* b) {
        Width = w;
        Height = h;
        Channels = c;
        Bytes = b;
    }

    int Width;
    int Height;
    int Channels;
    unsigned char* Bytes;
};

#endif