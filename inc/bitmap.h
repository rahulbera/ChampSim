#ifndef BITMAP_H
#define BITMAP_H

#include <bitset>
#include <string>
#define BITMAP_MAX_SIZE 64

typedef std::bitset<BITMAP_MAX_SIZE> Bitmap;

class BitmapHelper
{
public:
	static std::string to_string(Bitmap bmp);
	static uint32_t count_bits_set(Bitmap bmp);
};

#endif /* BITMAP_H */