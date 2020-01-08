#include "bitmap.h"

std::string BitmapHelper::to_string(Bitmap bmp)
{
	return bmp.to_string<char,std::string::traits_type,std::string::allocator_type>();
}

uint32_t BitmapHelper::count_bits_set(Bitmap bmp)
{
	return static_cast<uint32_t>(bmp.count());
}

uint32_t BitmapHelper::count_bits_same(Bitmap bmp1, Bitmap bmp2)
{
	uint32_t count_same = 0;
	for(uint32_t index = 0; index < BITMAP_MAX_SIZE; ++index)
	{
		if(bmp1[index] && bmp1[index] == bmp2[index])
		{
			count_same++;
		}
	}
	return count_same;
}

uint32_t BitmapHelper::count_bits_diff(Bitmap bmp1, Bitmap bmp2)
{
	uint32_t count_diff = 0;
	for(uint32_t index = 0; index < BITMAP_MAX_SIZE; ++index)
	{
		if(bmp1[index] && !bmp2[index])
		{
			count_diff++;
		}
	}
	return count_diff;
}