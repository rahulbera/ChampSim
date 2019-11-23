#ifndef SPP_H
#define SPP_H

#include <deque>
#include <vector>
#include "prefetcher.h"

using namespace std;

class STEntry
{
public:
	uint64_t page;
	uint32_t last_offset;
	uint64_t signature;
	uint32_t age;

public:
	void reset()
	{
		page = 0xdeadbeef;
		last_offset = 0;
		signature = 0xdeadbeef;
		age = 0;
	}
	STEntry(){reset();}
	~STEntry(){}
	string to_string();
};

class Outcome
{
public:
	int32_t delta;
	uint64_t occurence;

public:
	void reset()
	{
		delta = 0;
		occurence = 0;
	}
	Outcome(){reset();}
	~Outcome(){}
};

class PTEntry
{
public:
	uint64_t signature;
	uint64_t occurence;
	deque<Outcome*> outcomes;
	uint32_t age;
	string to_string();

public:
	void reset()
	{
		signature = 0xdeadbeef;
		occurence = 0;
		outcomes.clear();
		age = 0;
	}
	PTEntry(){reset();}
	~PTEntry(){}
	void update_delta(int32_t delta);
	void get_satisfying_deltas(double confidence, vector<int32_t> &satisfying_deltas, int32_t &max_delta, uint32_t &max_delta_occur);
};

class PFEntry
{
public:
	uint64_t address;
	bool demand_hit;
	bool from_ghr;

public:
	void reset()
	{
		address = 0xdeadbeef;
		demand_hit = false;
		from_ghr = false;
	}
	PFEntry(){reset();}
	~PFEntry(){};
};

class GHREntry
{
public:
	uint64_t signature;
	uint32_t offset;
	int32_t delta;
	double confidence;

public:
	void reset()
	{
		signature = 0xdeadbeef;
		offset = 0;
		delta = 0;
		confidence = 0.0;
	}
	GHREntry(){reset();}
	~GHREntry(){}
};

class SPP : public Prefetcher
{
private:
	deque<STEntry*> signature_table;
	deque<PTEntry*> pattern_table; /* TODO: make it set associative */
	deque<PFEntry*> prefetch_filter;
	deque<uint64_t> pref_buffer;
	deque<GHREntry*> ghr;

	double alpha;
	uint64_t total_pref, used_pref, demand_counter;

	/* TODO: implement GHR */

	struct
	{
		struct
		{
			uint64_t lookup;
			uint64_t hit;
			uint64_t insert;
			uint64_t evict;
		} st;

		struct
		{
			uint64_t lookup;
			uint64_t hit;
			uint64_t insert;
			uint64_t evict;
		} pt;

		struct
		{
			uint64_t called[2];
			uint64_t pref_generated[2];
			uint64_t pt_miss[2];
			uint64_t depth;
		} generate_prefetch;

		struct
		{
			uint64_t lookup[2];
			uint64_t hit[2];
			uint64_t issue_pref[2];
			uint64_t insert[2];
			uint64_t evict[2];
			uint64_t demand_seen_unique[2];
			uint64_t demand_seen[2];
			uint64_t evict_not_demanded[2];
		} pref_filter;

		struct
		{
			uint64_t buffered;
			uint64_t spilled;
			uint64_t issued;
		} pref_buffer;

		struct
		{
			uint64_t lookup;
			uint64_t hit;
			uint64_t insert;
			uint64_t evict;
		} ghr;

		struct
		{
			uint64_t update;
		} alpha;

	} stats;

private:
	void init_knobs();
	void init_stats();

	deque<STEntry*>::iterator search_signature_table(uint64_t page);
	void update_age_signature_table(deque<STEntry*>::iterator st_index);
	void insert_signature_table(uint64_t page, uint32_t offset, uint64_t signature);
	deque<STEntry*>::iterator search_victim_signature_table();
	void evict_signature_table(deque<STEntry*>::iterator victim);

	void update_pattern_table(uint64_t signature, int32_t delta);
	void insert_pattern_table(uint64_t signature, int32_t delta);
	deque<PTEntry*>::iterator search_pattern_table(uint64_t signature);
	deque<PTEntry*>::iterator search_victim_pattern_table();
	void evict_pattern_table(deque<PTEntry*>::iterator victim);
	void update_age_pattern_table(deque<PTEntry*>::iterator pt_index);

	deque<PFEntry*>::iterator search_prefetch_filter(uint64_t address);
	void insert_prefetch_filter(uint64_t address, bool from_ghr);
	deque<PFEntry*>::iterator search_victim_prefetch_filter();
	void evict_prefetch_filter(deque<PFEntry*>::iterator victim);
	void register_demand_hit(uint64_t address);

	void generate_prefetch(uint64_t page, uint32_t offset, uint64_t signature, double confidence, vector<uint64_t> &pref_addr, bool from_ghr);
	void filter_prefetch(vector<uint64_t> tmp_pref_addr, vector<uint64_t> &pref_addr, bool from_ghr);
	void buffer_prefetch(vector<uint64_t> pref_addr);
	void issue_prefetch(vector<uint64_t> &pref_addr);

	double compute_confidence(double prev_conf, double curr_ratio);
	int32_t compute_delta(uint32_t curr_offset, uint32_t prev_offset);
	uint64_t compute_signature(uint64_t old_sig, int32_t new_delta);

	void insert_ghr(uint64_t signature, uint32_t offset, int32_t delta, double confidence);
	bool lookup_ghr(uint32_t offset, uint64_t &signature, double &confidence);
	uint32_t compute_congruent_offset(uint32_t offset, int32_t delta);

public:
	SPP(string pref_type);
	~SPP();
	void invoke_prefetcher(uint64_t pc, uint64_t address, vector<uint64_t> &pref_addr);
	void dump_stats();
	void print_config();
};

inline void incr_counter(uint64_t &counter, uint64_t max_value) {if(counter < max_value) counter++;}

#endif /* SPP_H */

