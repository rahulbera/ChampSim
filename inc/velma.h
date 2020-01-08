#ifndef VELMA_H
#define VELMA_H

#include <vector>
#include "prefetcher.h"
#include "learning_engine.h"

using namespace std;

#define MAX_CANDIDATE_PREFETCHERS 8

class Velma : public Prefetcher
{
private:
	vector<Prefetcher*> m_prefetchers;

	/* staistics */
	struct
	{
		struct
		{
			uint64_t called;
			uint64_t called_dist[MAX_CANDIDATE_PREFETCHERS];
			uint64_t pref_dist[MAX_CANDIDATE_PREFETCHERS];
		} invoke_prefetcher;

		struct
		{
			uint64_t pref_dist[MAX_CANDIDATE_PREFETCHERS];
		} issue;

	} stats;

private:
	void init_knobs();
	void init_stats();
	void select_prefetch(vector<vector<uint64_t> > pref_candidates, vector<uint64_t> &pref_addr);

public:
	Velma(string type);
	~Velma();
	void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);
	void register_fill(uint64_t address);
	void register_prefetch_hit(uint64_t address);
	void dump_stats();
	void print_config();
};


#endif /* VELMA_H */

