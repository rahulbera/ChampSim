#include <assert.h>
#include <iostream>
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
	this->unique_pcs.insert(pc);

	/* insert deltas */
	if(!this->offsets.empty())
	{
		int32_t delta = (offset > this->offsets.back()) ? (offset - this->offsets.back()) : (-1)*(this->offsets.back() - offset);
		if(this->deltas.size() >= knob::scooby_max_deltas)
		{
			this->deltas.pop_front();
		}
		this->deltas.push_back(delta);
		this->unique_deltas.insert(delta);
	}

	/* insert offset */
	if(this->offsets.size() >= knob::scooby_max_offsets)
	{
		this->offsets.pop_front();
	}
	this->offsets.push_back(offset);

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

void ScoobyRecorder::record_access(uint64_t pc, uint64_t address, uint64_t page, uint32_t offset)
{
	unique_pcs.insert(pc);
	unique_pages.insert(page);
}

void ScoobyRecorder::record_trigger_access(uint64_t page, uint64_t pc, uint32_t offset)
{
	unique_trigger_pcs.insert(pc);
}

void ScoobyRecorder::dump_stats()
{
	cout << "unique_pcs " << unique_pcs.size() << endl
		<< "unique_pages " << unique_pages.size() << endl
		<< "unique_trigger_pcs " << unique_trigger_pcs.size() << endl
		<< endl;
}

void print_access_debug(Scooby_STEntry *stentry)
{
	uint64_t trigger_pc = stentry->pcs.front();
	/*some streaming PC of 432.milc-274B*/
	// if(trigger_pc != 0x40e7b8 && trigger_pc != 0x40e7ad) 
	// {
	// 	return;
	// }
	uint32_t trigger_offset = stentry->offsets.front();
	uint32_t unique_pc_count = stentry->unique_pcs.size();
	fprintf(stdout, "[ACCESS] %16lx|%16lx|%2u|%2u|%64s|%2u|", stentry->page, 
															trigger_pc, 
															trigger_offset, 
															unique_pc_count, 
															BitmapHelper::to_string(stentry->pattern).c_str(), 
															BitmapHelper::count_bits_set(stentry->pattern));
	for (const uint64_t& pc: stentry->unique_pcs)
	{
		fprintf(stdout, "%lx,", pc);
	}
	fprintf(stdout, "|");
	for (const int32_t& delta: stentry->unique_deltas)
	{
		fprintf(stdout, "%d,", delta);
	}
	fprintf(stdout, "\n");
}
