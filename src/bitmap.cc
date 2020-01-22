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

uint64_t BitmapHelper::value(Bitmap bmp)
{
	return bmp.to_ullong();
}

Bitmap BitmapHelper::rotate_left(Bitmap bmp, uint32_t amount)
{
	Bitmap result;
	for(uint32_t index = 0; index < BITMAP_MAX_SIZE - amount; ++index)
	{
		result[index+amount] = bmp[index];
	}
	for(uint32_t index = 0; index < amount; ++index)
	{
		result[index] = bmp[index+BITMAP_MAX_SIZE-amount];
	}
	return result;
}

Bitmap BitmapHelper::rotate_right(Bitmap bmp, uint32_t amount)
{
	Bitmap result;
	for(uint32_t index = 0; index < BITMAP_MAX_SIZE - amount; ++index)
	{
		result[index] = bmp[index+amount];
	}
	for(uint32_t index = 0; index < amount; ++index)
	{
		result[BITMAP_MAX_SIZE-amount+index] = bmp[index];
	}
	return result;

}