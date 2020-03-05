#ifndef SPP_DEV2_H
#define SPP_DEV2_H

#include <string>
#include "prefetcher.h"
#include "spp_dev2_helper.h"
#include "cache.h"

/* SPP Prefetcher */
class SPP_dev2 : public Prefetcher
{
private:
	CACHE *m_parent_cache;
	SIGNATURE_TABLE ST;
	PATTERN_TABLE   PT;
	PREFETCH_FILTER FILTER;
	GLOBAL_REGISTER GHR;

private:
	void init_knobs();
	void init_stats();

public:
	SPP_dev2(std::string type, CACHE *cache);
	~SPP_dev2();
	void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, std::vector<uint64_t> &pref_addr);
	void dump_stats();
	void print_config();
	void cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr);
};

#endif /* SPP_DEV2_H */

