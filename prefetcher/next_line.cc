#include <vector>
#include "next_line.h"
#include "champsim.h"

namespace knob
{
	extern int32_t next_line_delta;
}

void NextLinePrefetcher::init_knobs()
{

}

void NextLinePrefetcher::init_stats()
{
	bzero(&stats, sizeof(stats));
}

void NextLinePrefetcher::print_config()
{
	cout << "next_line_delta " << knob::next_line_delta
		<< endl;
}

NextLinePrefetcher::NextLinePrefetcher(string type) : Prefetcher(type)
{

}

NextLinePrefetcher::~NextLinePrefetcher()
{

}

void NextLinePrefetcher::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
	uint64_t page = address >> LOG2_PAGE_SIZE;
	uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

	int32_t prefetched_offset = offset + knob::next_line_delta;
	if(prefetched_offset >= 0 && prefetched_offset < 64)
	{
		uint64_t addr = (page << LOG2_PAGE_SIZE) + (prefetched_offset << LOG2_BLOCK_SIZE);
		pref_addr.push_back(addr); 
		stats.predict.total++;
	}
	else
	{
		stats.predict.out_of_bounds++;
	}
}

void NextLinePrefetcher::dump_stats()
{
	cout << "next_line_predict_total " << stats.predict.total
		<< "next_line_predict_out_of_bounds " << stats.predict.out_of_bounds
		<< endl;
}