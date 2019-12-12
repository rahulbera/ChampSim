#include "bitmap.h"

std::string BitmapHelper::to_string(Bitmap bmp)
{
	return bmp.to_string<char,std::string::traits_type,std::string::allocator_type>();
}

uint32_t BitmapHelper::count_bits_set(Bitmap bmp)
{
	return static_cast<uint32_t>(bmp.count());
}