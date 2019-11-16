#include "bitmap.h"

std::string BitmapHelper::to_string(Bitmap bmp)
{
	return bmp.to_string<char,std::string::traits_type,std::string::allocator_type>();
}