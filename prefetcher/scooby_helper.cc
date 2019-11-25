#include <assert.h>
#include "scooby_helper.h"

#define DELTA_SIG_MAX_BITS 12
#define DELTA_SIG_SHIFT 3
#define PC_SIG_MAX_BITS 16
#define PC_SIG_SHIFT 4

namespace knob
{
	extern uint32_t scooby_max_pcs;
	extern uint32_t scooby_max_offsets;
	extern uint32_t scooby_max_deltas;
}

void Scooby_STEntry::update(uint64_t page, uint64_t pc, uint32_t offset, uint64_t address)
{
	assert(this->page == page);

	/* insert PC */
	if(this->pcs.size() >= knob::scooby_max_pcs)
	{
		this->pcs.pop_front();
	}
	this->pcs.push_back(pc);

	/* insert offset */
	if(this->offsets.size() >= knob::scooby_max_offsets)
	{
		this->offsets.pop_front();
	}
	this->offsets.push_back(offset);

	/* insert deltas */
	if(!this->offsets.empty())
	{
		int32_t delta = (offset > this->offsets.back()) ? (offset - this->offsets.back()) : (-1)*(this->offsets.back() - offset);
		if(this->deltas.size() >= knob::scooby_max_deltas)
		{
			this->deltas.pop_front();
		}
		this->deltas.push_back(delta);		
	}

	/* update pattern */
	this->pattern[offset] = 1;
}

uint32_t Scooby_STEntry::get_delta_sig()
{
	uint32_t signature = 0;
	uint32_t delta = 0;

	/* compute signature only using last 4 deltas */
	uint32_t n = deltas.size();
	uint32_t ptr = (n >= 4) ? (n - 4) : 0;
	
	for(uint32_t index = ptr; index < deltas.size(); ++index)
	{
		signature = (signature << DELTA_SIG_SHIFT);
		signature = signature & ((1ull << DELTA_SIG_MAX_BITS) - 1);
		delta = (uint32_t)(deltas[index] & ((1ull << 7) - 1));
		signature = (signature ^ delta);
		signature = signature & ((1ull << DELTA_SIG_MAX_BITS) - 1);
	}
	return signature;
}

uint32_t Scooby_STEntry::get_pc_sig()
{
	uint32_t signature = 0;
	uint32_t pc = 0;

	/* compute signature only using last 4 deltas */
	uint32_t n = pcs.size();
	uint32_t ptr = (n >= 4) ? (n - 4) : 0;
	
	for(uint32_t index = ptr; index < pcs.size(); ++index)
	{
		signature = (signature << PC_SIG_SHIFT);
		signature = signature & ((1ull << PC_SIG_MAX_BITS) - 1);
		pc = (uint32_t)(pcs[index] & ((1ull << 7) - 1));
		signature = (signature ^ pc);
		signature = signature & ((1ull << PC_SIG_MAX_BITS) - 1);
	}
	return signature;
}