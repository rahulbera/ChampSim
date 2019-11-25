#ifndef SCOOBY_HELPER_H
#define SCOOBY_HELPER_H

#include <deque>
#include "bitmap.h"

using namespace std;

class Scooby_STEntry
{
public:
	uint64_t page;
	deque<uint64_t> pcs;
	deque<uint32_t> offsets;
	deque<int32_t> deltas;
	Bitmap pattern;

public:
	void reset()
	{
		page = 0xdeadbeef;
		pcs.clear();
		offsets.clear();
		deltas.clear();
		pattern.reset();
	}
	Scooby_STEntry(uint64_t pc, uint32_t offset)
	{
		reset();
		pcs.push_back(pc);
		offsets.push_back(offset);
	}
	~Scooby_STEntry(){}
	uint32_t get_delta_sig();
	uint32_t get_pc_sig();
	void update(uint64_t page, uint64_t pc, uint32_t offset, uint64_t address);
};

#endif /* SCOOBY_HELPER_H */

