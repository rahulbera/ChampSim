#ifndef NEXT_LINE
#define NEXT_LINE

#include "prefetcher.h"
using namespace std;

class NextLinePrefetcher : public Prefetcher
{
private:
	struct
	{
		struct
		{
			uint64_t total;
			uint64_t out_of_bounds;
		} predict;
	} stats;

private:
	void init_knobs();
	void init_stats();

public:
	NextLinePrefetcher(string type);
	~NextLinePrefetcher();
	void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);
	void dump_stats();
	void print_config();
};


#endif /* NEXT_LINE */

