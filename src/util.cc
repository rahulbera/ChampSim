#include <cassert>
#include "util.h"

uint32_t folded_xor(uint64_t value, uint32_t num_folds)
{
	assert(num_folds > 1);
	assert((num_folds ^ (num_folds-1)) == 0); /* has to be power of 2 */
	uint32_t mask = 0;
	uint32_t bits_in_fold = 64/num_folds;
	if(num_folds == 2)
	{
		mask = 0xffffffff;
	}
	else
	{
		mask = (1ul << bits_in_fold) - 1;
	}
	uint32_t folded_value = 0;
	for(uint32_t fold = 0; fold < num_folds; ++fold)
	{
		folded_value = folded_value ^ ((value >> (fold * bits_in_fold)) & mask);
	}
	return folded_value;
}