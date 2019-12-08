#include <stdio.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <strings.h>
#include "next_line.h"
#include "champsim.h"

namespace knob
{
	extern int32_t  next_line_delta;
	extern uint32_t next_line_pt_size;
	extern bool     next_line_enable_prefetch_tracking;
	extern bool     next_line_enable_trace;
	extern uint32_t next_line_trace_interval;
	extern std::string next_line_trace_name;
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
	cout << "next_line_delta " << knob::next_line_delta << endl
		<< "next_line_pt_size " << knob::next_line_pt_size << endl
		<< "next_line_enable_prefetch_tracking " << knob::next_line_enable_prefetch_tracking << endl
		<< endl;
}

NextLinePrefetcher::NextLinePrefetcher(string type) : Prefetcher(type)
{
	init_knobs();
	init_stats();

	if(knob::next_line_enable_trace)
	{
		trace_timestamp = 0;
		trace_interval = 0;
		trace = fopen(knob::next_line_trace_name.c_str(), "w");
		assert(trace);
	}
}

NextLinePrefetcher::~NextLinePrefetcher()
{
	if(knob::next_line_enable_trace && trace)
	{
		fclose(trace);
	}
}

void NextLinePrefetcher::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
	uint64_t page = address >> LOG2_PAGE_SIZE;
	uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

	if(knob::next_line_enable_prefetch_tracking)
	{
		record_demand(address);
	}

	int32_t prefetched_offset = offset + knob::next_line_delta;
	if(prefetched_offset >= 0 && prefetched_offset < 64)
	{
		uint64_t addr = (page << LOG2_PAGE_SIZE) + (prefetched_offset << LOG2_BLOCK_SIZE);
		pref_addr.push_back(addr);
		stats.predict.total++;
		if(knob::next_line_enable_prefetch_tracking)
		{
			track(addr);
		}
	}
	else
	{
		stats.predict.out_of_bounds++;
	}
}

void NextLinePrefetcher::track(uint64_t address)
{
	stats.track.called++;
	if(search_pt(address) == NULL)
	{
		stats.track.pt_miss++;
		if(prefetch_tracker.size() >= knob::next_line_pt_size)
		{
			stats.track.evict++;
			NL_PTEntry *ptentry = prefetch_tracker.front();
			prefetch_tracker.pop_front();
			measure_stats(ptentry);
			delete ptentry;
		}
		prefetch_tracker.push_back(new NL_PTEntry(address, false));
		stats.track.insert++;
	}
	else
	{
		stats.track.pt_hit++;
	}
}

NL_PTEntry* NextLinePrefetcher::search_pt(uint64_t address)
{
	auto it = find_if(prefetch_tracker.begin(), prefetch_tracker.end(), [address](NL_PTEntry *ptentry){return ptentry->address == address;});
	return it != prefetch_tracker.end() ? (*it) : NULL;
}

void NextLinePrefetcher::register_fill(uint64_t address)
{
	if(!knob::next_line_enable_prefetch_tracking)
	{
		return;
	}

	stats.register_fill.called++;
	NL_PTEntry *ptentry = search_pt(address);
	if(ptentry)
	{
		stats.register_fill.fill++;
		ptentry->fill = true;
	}
}

void NextLinePrefetcher::record_demand(uint64_t address)
{
	stats.record_demand.called++;
	NL_PTEntry *ptentry = search_pt(address);
	if(ptentry)
	{
		stats.record_demand.hit++;
		if(ptentry->fill)
		{
			stats.record_demand.timely++;
			ptentry->timely = 1;
		}
		else
		{
			stats.record_demand.untimely++;
			ptentry->timely = 0;
		}
	}	
}

void NextLinePrefetcher::measure_stats(NL_PTEntry *ptentry)
{
	stats.pref.total++;
	switch(ptentry->timely)
	{
		case 1:
			stats.pref.timely++;
			break;
		case 0:
			stats.pref.untimely++;
			break;
		case -1:
			stats.pref.incorrect++;
			break;
		default:
			assert(false);
	}

	if(knob::next_line_enable_trace && trace_interval++ == knob::next_line_trace_interval)
	{
		trace_timestamp++;
		fprintf(trace, "%lu,%d\n", trace_timestamp, ptentry->timely);
		trace_interval = 0;
	}
}

void NextLinePrefetcher::dump_stats()
{
	cout << "next_line_predict_total " << stats.predict.total << endl
		<< "next_line_predict_out_of_bounds " << stats.predict.out_of_bounds << endl
		<< "next_line_track_called " << stats.track.called << endl
		<< "next_line_track_pt_miss " << stats.track.pt_miss << endl
		<< "next_line_track_pt_hit " << stats.track.pt_hit << endl
		<< "next_line_track_evict " << stats.track.evict << endl
		<< "next_line_track_insert " << stats.track.insert << endl
		<< "next_line_register_fill_called " << stats.register_fill.called << endl
		<< "next_line_register_fill_fill " << stats.register_fill.fill << endl
		<< "next_line_record_demand_called " << stats.record_demand.called << endl
		<< "next_line_record_demand_hit " << stats.record_demand.hit << endl
		<< "next_line_record_demand_timely " << stats.record_demand.timely << endl
		<< "next_line_record_demand_untimely " << stats.record_demand.untimely << endl
		<< "next_line_pref_total " << stats.pref.total << endl
		<< "next_line_pref_timely " << stats.pref.timely << endl
		<< "next_line_pref_untimely " << stats.pref.untimely << endl
		<< "next_line_pref_incorrect " << stats.pref.incorrect << endl
		<< endl;
}