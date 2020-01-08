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

	/* update demanded pattern */
	this->bmp_real[offset] = 1;
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

void Scooby_STEntry::track_prefetch(uint32_t pred_offset)
{
	if(!bmp_pred[pred_offset])
	{
		bmp_pred[pred_offset] = 1;
		total_prefetches++;
	}
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
	fprintf(stdout, "[ACCESS] %16lx|%16lx|%2u|%2u|%64s|%64s|%2u|%2u|%2u|%2u|%2u|", 
		stentry->page, 
		trigger_pc, 
		trigger_offset, 
		unique_pc_count, 
		BitmapHelper::to_string(stentry->bmp_real).c_str(), 
		BitmapHelper::to_string(stentry->bmp_pred).c_str(), 
		BitmapHelper::count_bits_set(stentry->bmp_real),
		BitmapHelper::count_bits_set(stentry->bmp_pred),
		BitmapHelper::count_bits_same(stentry->bmp_real, stentry->bmp_pred), /* covered */
		BitmapHelper::count_bits_diff(stentry->bmp_real, stentry->bmp_pred), /* uncovered */
		BitmapHelper::count_bits_diff(stentry->bmp_pred, stentry->bmp_real)  /* over prediction */
	);
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

uint32_t ScoobyHash::jenkins(uint32_t key)
{
    // Robert Jenkins' 32 bit mix function
    key += (key << 12);
    key ^= (key >> 22);
    key += (key << 4);
    key ^= (key >> 9);
    key += (key << 10);
    key ^= (key >> 2);
    key += (key << 7);
    key ^= (key >> 12);
    return key;
}

uint32_t ScoobyHash::knuth(uint32_t key)
{
    // Knuth's multiplicative method
    key = (key >> 3) * 2654435761;
    return key;
}

uint32_t ScoobyHash::murmur3(uint32_t key)
{
	/* TODO: define it using murmur3's finilization steps */
	assert(false);
}

/* originally ment for 32b key */
uint32_t ScoobyHash::jenkins32(uint32_t key)
{
   key = (key+0x7ed55d16) + (key<<12);
   key = (key^0xc761c23c) ^ (key>>19);
   key = (key+0x165667b1) + (key<<5);
   key = (key+0xd3a2646c) ^ (key<<9);
   key = (key+0xfd7046c5) + (key<<3);
   key = (key^0xb55a4f09) ^ (key>>16);
   return key;
}

/* originally ment for 32b key */
uint32_t ScoobyHash::hash32shift(uint32_t key)
{
	key = ~key + (key << 15); // key = (key << 15) - key - 1;
	key = key ^ (key >> 12);
	key = key + (key << 2);
	key = key ^ (key >> 4);
	key = key * 2057; // key = (key + (key << 3)) + (key << 11);
	key = key ^ (key >> 16);
	return key;
}

/* originally ment for 32b key */
uint32_t ScoobyHash::hash32shiftmult(uint32_t key)
{
	int c2=0x27d4eb2d; // a prime or an odd constant
	key = (key ^ 61) ^ (key >> 16);
	key = key + (key << 3);
	key = key ^ (key >> 4);
	key = key * c2;
	key = key ^ (key >> 15);
	return key;
}

uint32_t ScoobyHash::hash64shift(uint32_t key)
{
	key = (~key) + (key << 21); // key = (key << 21) - key - 1;
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); // key * 265
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); // key * 21
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
}

uint32_t ScoobyHash::hash5shift(uint32_t key)
{
	key = (key ^ 61) ^ (key >> 16);
    key = key + (key << 3);
    key = key ^ (key >> 4);
    key = key * 0x27d4eb2d;
    key = key ^ (key >> 15);
    return key;
}

/* hash6shift is jenkin32 */

uint32_t ScoobyHash::hash7shift(uint32_t key)
{
    key -= (key << 6);
    key ^= (key >> 17);
    key -= (key << 9);
    key ^= (key << 4);
    key -= (key << 3);
    key ^= (key << 10);
    key ^= (key >> 15);
    return key ;
}

/* use low bit values */
uint32_t ScoobyHash::Wang6shift(uint32_t key)
{
    key += ~(key << 15);
    key ^=  (key >> 10);
    key +=  (key << 3);
    key ^=  (key >> 6);
    key += ~(key << 11);
    key ^=  (key >> 16);
    return key;
}

uint32_t ScoobyHash::Wang5shift(uint32_t key)
{
    key = (key + 0x479ab41d) + (key << 8);
    key = (key ^ 0xe4aa10ce) ^ (key >> 5);
    key = (key + 0x9942f0a6) - (key << 14);
    key = (key ^ 0x5aedd67d) ^ (key >> 3);
    key = (key + 0x17bea992) + (key << 7);
    return key;
}

uint32_t ScoobyHash::Wang4shift( uint32_t key)
{
    key = (key ^ 0xdeadbeef) + (key << 4);
    key = key ^ (key >> 10);
    key = key + (key << 7);
    key = key ^ (key >> 13);
    return key;
}

uint32_t ScoobyHash::Wang3shift( uint32_t key)
{
    key = key ^ (key >> 4);
    key = (key ^ 0xdeadbeef) + (key << 5);
    key = key ^ (key >> 11);
    return key;
}

uint32_t ScoobyHash::hybrid1(uint32_t key)
{
	return knuth(jenkins(key));
}