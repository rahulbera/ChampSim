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
	static uint32_t count_bits_same(Bitmap bmp1, Bitmap bmp2);
	static uint32_t count_bits_diff(Bitmap bmp1, Bitmap bmp2);
	static uint64_t value(Bitmap bmp);
	static Bitmap rotate_left(Bitmap bmp, uint32_t amount);
	static Bitmap rotate_right(Bitmap bmp, uint32_t amount);
};

#endif /* BITMAP_H */