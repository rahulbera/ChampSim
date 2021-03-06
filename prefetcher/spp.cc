#include <iostream>
#include <algorithm>
#include <sstream>
#include <strings.h>
#include <iomanip>
#include "spp.h"
#include "champsim.h"

#define FIXED_FLOAT(x) std::fixed << std::setprecision(2) << (x)

using namespace std;

namespace knob
{
	extern uint32_t spp_st_size;
	extern uint32_t spp_pt_size;
	extern uint32_t spp_max_outcomes;
	extern double   spp_max_confidence;
	extern uint32_t spp_max_depth;
	extern uint32_t spp_max_prefetch_per_level;
	extern uint32_t spp_max_confidence_counter_value;
	extern uint32_t spp_max_global_counter_value;
	extern uint32_t spp_pf_size;
	extern bool     spp_enable_alpha;
	extern bool     spp_enable_pref_buffer;
	extern uint32_t spp_pref_buffer_size;
	extern uint32_t spp_pref_degree;
	extern bool     spp_enable_ghr;
	extern uint32_t spp_ghr_size;
	extern uint32_t spp_signature_bits;
	extern uint32_t spp_alpha_epoch;
}

void SPP::init_knobs()
{

}

void SPP::init_stats()
{
	bzero(&stats, sizeof(stats));
}

void SPP::print_config()
{
	cout << "spp_st_size " << knob::spp_st_size << endl
		<< "spp_pt_size " << knob::spp_pt_size << endl
		<< "spp_max_outcomes " << knob::spp_max_outcomes << endl
		<< "spp_max_confidence " << knob::spp_max_confidence << endl
		<< "spp_max_depth " << knob::spp_max_depth << endl
		<< "spp_max_prefetch_per_level " << knob::spp_max_prefetch_per_level << endl
		<< "spp_max_confidence_counter_value " << knob::spp_max_confidence_counter_value << endl
		<< "spp_max_global_counter_value " << knob::spp_max_global_counter_value << endl
		<< "spp_pf_size " << knob::spp_pf_size << endl
		<< "spp_enable_alpha " << knob::spp_enable_alpha << endl
		<< "spp_enable_pref_buffer " << knob::spp_enable_pref_buffer << endl
		<< "spp_pref_buffer_size " << knob::spp_pref_buffer_size << endl
		<< "spp_pref_degree " << knob::spp_pref_degree << endl
		<< "spp_enable_ghr " << knob::spp_enable_ghr << endl
		<< "spp_ghr_size " << knob::spp_ghr_size << endl
		<< "spp_signature_bits " << knob::spp_signature_bits << endl
		<< "spp_alpha_epoch " << knob::spp_alpha_epoch << endl
		<< endl;
}

SPP::SPP(string type) : Prefetcher(type)
{
	init_knobs();
	init_stats();

	alpha = 1.0;
	total_pref = 0;
	used_pref = 0;
	demand_counter = 0;
}

SPP::~SPP()
{

}

void SPP::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
	uint64_t page = address >> LOG2_PAGE_SIZE;
	uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);
	vector<uint64_t> tmp_pref_addr;

	// cout << "[ACCESS]"
	// 	<< "pc: " << hex << setw(10) << pc
	// 	<< " page: " << hex << setw(10) << page
	// 	<< " offset: " << dec << setw(2) << offset
	// 	<< endl;

	/* register demand hit in PF */
	if(knob::spp_enable_alpha)
	{
		register_demand_hit(address);
	}

	auto st_index = search_signature_table(page);
	stats.st.lookup++;
	if(st_index != signature_table.end())
	{
		stats.st.hit++;
		STEntry *stentry = (*st_index);
		int32_t curr_delta = compute_delta(offset, stentry->last_offset);
		uint64_t new_signature = compute_signature(stentry->signature, curr_delta);

		/* update PT */
		update_pattern_table(stentry->signature, curr_delta);

		/* update ST */
		// cout << "    [ST_b] " << stentry->to_string() << endl;
		stentry->signature = new_signature;
		stentry->last_offset = offset;
		update_age_signature_table(st_index);
		// cout << "    [ST_a] " << stentry->to_string() << endl;
		
		/* use new signature to generate prefetch */
		generate_prefetch(page, offset, new_signature, 1.0, tmp_pref_addr, false);
		filter_prefetch(tmp_pref_addr, pref_addr, false);
		// cout << "    [Pref] gen: " << tmp_pref_addr.size() << " filtered: " << (tmp_pref_addr.size() - pref_addr.size()) << endl;
		if(knob::spp_enable_pref_buffer)
		{
			buffer_prefetch(pref_addr);
			pref_addr.clear();
		}
	}
	else
	{
		uint64_t signature = 0x0;
		if(knob::spp_enable_ghr)
		{
			double confidence = 0.0;
			stats.ghr.lookup++;
			if(lookup_ghr(offset, signature, confidence))
			{
				stats.ghr.hit++;
				generate_prefetch(page, offset, signature, confidence, tmp_pref_addr, true);
				filter_prefetch(tmp_pref_addr, pref_addr, true);
				// cout << "    [Pref] gen: " << tmp_pref_addr.size() << " filtered: " << (tmp_pref_addr.size() - pref_addr.size()) << endl;
				if(knob::spp_enable_pref_buffer)
				{
					buffer_prefetch(pref_addr);
					pref_addr.clear();
				}
			}
		}
		insert_signature_table(page, offset, signature);
	}

	/* slowly inject prefetches at every demand access, if buffer is turned on */
	if(knob::spp_enable_pref_buffer)
	{
		issue_prefetch(pref_addr);
		// cout << "    [Issu] count: " << pref_addr.size() << endl;
	}

	// cout << "    [Gcnt] C_total: " << dec << total_pref << " C_used: " << dec << used_pref << " alpha: " << FIXED_FLOAT(alpha) << endl;
}

/* Functions for Signature Table */
deque<STEntry*>::iterator SPP::search_signature_table(uint64_t page)
{
	return find_if(signature_table.begin(), signature_table.end(), [page](STEntry* stentry){return (stentry->page == page);});
}

void SPP::update_age_signature_table(deque<STEntry*>::iterator st_index)
{
	for(auto it = signature_table.begin(); it != signature_table.end(); ++it)
	{
		(*it)->age++;
	}
	(*st_index)->age = 0;
}

void SPP::insert_signature_table(uint64_t page, uint32_t offset, uint64_t signature)
{
	stats.st.insert++;
	if(signature_table.size() >= knob::spp_st_size)
	{
		auto victim = search_victim_signature_table();
		evict_signature_table(victim);
	}

	STEntry *stentry = new STEntry();
	stentry->page = page;
	stentry->last_offset = offset;
	stentry->signature = signature;
	stentry->age = 0;
	for(uint32_t index = 0; index < signature_table.size(); ++index) signature_table[index]->age++;
	signature_table.push_back(stentry);	
	// cout << "    [ST_insert] " << stentry->to_string() << endl;
}

deque<STEntry*>::iterator SPP::search_victim_signature_table()
{
	deque<STEntry*>::iterator it, victim;
	uint32_t max_age = 0;
	for(it = signature_table.begin(); it != signature_table.end(); ++it)
	{
		if((*it)->age > max_age)
		{
			max_age = (*it)->age;
			victim = it;
		}
	}
	return victim;
}

void SPP::evict_signature_table(deque<STEntry*>::iterator victim)
{
	stats.st.evict++;
	STEntry *stentry = (*victim);
	signature_table.erase(victim);
	delete stentry;
}

/* Functions for Pattern Table */
void SPP::update_pattern_table(uint64_t signature, int32_t delta)
{
	stats.pt.lookup++;
	PTEntry *ptentry = NULL;
	auto pt_index = search_pattern_table(signature);
	if(pt_index != pattern_table.end())
	{
		stats.pt.hit++;
		ptentry = (*pt_index);
		// cout << "    [PT_b] " << ptentry->to_string() << endl;
		ptentry->update_delta(delta);
		incr_counter(ptentry->occurence, knob::spp_max_confidence_counter_value);
		update_age_pattern_table(pt_index);
		// cout << "    [PT_a] " << ptentry->to_string() << endl;
	}
	else
	{
		insert_pattern_table(signature, delta);
	}
}

void SPP::insert_pattern_table(uint64_t signature, int32_t delta)
{
	stats.pt.insert++;
	if(pattern_table.size() >= knob::spp_pt_size)
	{
		auto victim = search_victim_pattern_table();
		evict_pattern_table(victim);
	}

	PTEntry *ptentry = new PTEntry();
	ptentry->signature = signature;
	incr_counter(ptentry->occurence, knob::spp_max_confidence_counter_value);
	ptentry->update_delta(delta);
	ptentry->age = 0;
	for(uint32_t index = 0; index < pattern_table.size(); ++index) pattern_table[index]->age++;
	pattern_table.push_back(ptentry);
	// cout << "    [PT_insert] " << ptentry->to_string() << endl;
}

deque<PTEntry*>::iterator SPP::search_pattern_table(uint64_t signature)
{
	return find_if(pattern_table.begin(), pattern_table.end(), [signature](PTEntry *ptentry){return (ptentry->signature = signature);});
}

deque<PTEntry*>::iterator SPP::search_victim_pattern_table()
{
	deque<PTEntry*>::iterator it, victim;
	uint32_t max_age = 0;
	for(it = pattern_table.begin(); it != pattern_table.end(); ++it)
	{
		if((*it)->age > max_age)
		{
			max_age = (*it)->age;
			victim = it;
		}
	}
	return victim;	
}

void SPP::evict_pattern_table(deque<PTEntry*>::iterator victim)
{
	stats.pt.evict++;
	PTEntry *ptentry = (*victim);
	pattern_table.erase(victim);
	delete ptentry;
}

void SPP::update_age_pattern_table(deque<PTEntry*>::iterator pt_index)
{
	for(auto it = pattern_table.begin(); it != pattern_table.end(); ++it)
	{
		(*it)->age++;
	}
	(*pt_index)->age = 0;
}

/* PTEntry functions */
void PTEntry::update_delta(int32_t delta)
{
	Outcome *outcome = NULL;
	auto oindex = find_if(outcomes.begin(), outcomes.end(), [delta](Outcome *outcome){return (outcome->delta == delta);});
	if(oindex != outcomes.end())
	{
		incr_counter((*oindex)->occurence, knob::spp_max_confidence_counter_value);
	}
	else
	{
		if(outcomes.size() >= knob::spp_max_outcomes)
		{
			outcome = outcomes.back();
			outcomes.pop_back();
			delete outcome;
		}

		outcome = new Outcome();
		outcome->delta = delta;
		outcome->occurence = 1;
		outcomes.push_back(outcome);
	}
	sort(outcomes.begin(), outcomes.end(), [](Outcome *o1, Outcome *o2){return (o1->occurence >= o2->occurence);});
}

void PTEntry::get_satisfying_deltas(double confidence, vector<int32_t> &satisfying_deltas, int32_t &max_delta, uint32_t &max_delta_occur)
{
	assert(outcomes.size() != 0);
	max_delta = outcomes.front()->delta;
	max_delta_occur = outcomes.front()->occurence;
	for(uint32_t index = 0; index < outcomes.size(); ++index)
	{
		double conf = confidence * ((double)outcomes[index]->occurence / occurence);
		if((100*conf) >= knob::spp_max_confidence)
		{
			satisfying_deltas.push_back(outcomes[index]->delta);
		}
		else
		{
			break; /* as the list is already sorted by decreseaing order of occurence */
		}
	}
}

string PTEntry::to_string()
{
	stringstream ss;
	ss << "sig: " << hex << setw(12) << signature
		<< " occ: " << dec << setw(3) << occurence
		<< " age: " << dec << setw(4) << age << " "
		;
	for(uint32_t index = 0; index < outcomes.size(); ++index)
	{
		ss << "(" << outcomes[index]->delta << "," << outcomes[index]->occurence << ")";
	}
	return ss.str();
}

/* STEntry functions */
string STEntry::to_string()
{
	stringstream ss;
	ss << "page: " << hex << setw(10) << page
		<< " offset: " << dec << setw(2) << last_offset
		<< " sig: " << hex << setw(12) << signature
		<< " age: " << dec << setw(4) << age
		;
	return ss.str(); 
}

void SPP::generate_prefetch(uint64_t page, uint32_t offset, uint64_t signature, double confidence, vector<uint64_t> &pref_addr, bool from_ghr)
{
	double curr_confidence = confidence;
	uint32_t depth = 0;
	int32_t curr_offset = (int32_t)offset, pref_offset = 0;
	bool crossed_page = false;
	/* tracked for debugging purpose */
	vector<int32_t> pref_offsets, pref_offsets_discarded;
	vector<double> conf, conf_discarded;

	// cout << "    [Pgen] page: " << hex << setw(10) << page
	// 	<< " offset " << dec << setw(2) << offset
	// 	<< " sig: " << hex << setw(12) << signature
	// 	<< " conf: " << dec << setw(4) << FIXED_FLOAT(confidence)
	// 	<< endl; 

	stats.generate_prefetch.called[from_ghr]++;

	while((100*curr_confidence) >= knob::spp_max_confidence && depth < knob::spp_max_depth)
	{
		auto pt_index = search_pattern_table(signature);
		if(pt_index == pattern_table.end())
		{
			break;
		}

		PTEntry *ptentry = (*pt_index);
		vector<int32_t> satisfying_deltas;
		int32_t max_delta = 0;
		uint32_t max_delta_occur = 0;
		ptentry->get_satisfying_deltas(curr_confidence, satisfying_deltas, max_delta, max_delta_occur);

		for(uint32_t index = 0; index < satisfying_deltas.size() && index < knob::spp_max_prefetch_per_level; ++index)
		{
			pref_offset = curr_offset + satisfying_deltas[index];
			if(pref_offset >= 0 && pref_offset < 64 && pref_offset != (int32_t)offset)
			{
				pref_offsets.push_back(pref_offset);
				conf.push_back(curr_confidence);
				uint64_t addr = (page << LOG2_PAGE_SIZE) + (((uint32_t)pref_offset) << LOG2_BLOCK_SIZE);
				pref_addr.push_back(addr);
				stats.generate_prefetch.pref_generated[from_ghr]++;
			}
			else
			{
				conf_discarded.push_back(curr_confidence);
				pref_offsets_discarded.push_back(pref_offset);
			}
		}

		curr_confidence = compute_confidence(curr_confidence, (double)max_delta_occur/ptentry->occurence);
		if(knob::spp_enable_ghr && !crossed_page && (curr_offset+max_delta < 0 || curr_offset+max_delta >= 64))
		{
			crossed_page = true;
			insert_ghr(signature, (uint32_t)curr_offset, max_delta, curr_confidence);
		}
		signature = compute_signature(signature, max_delta);
		curr_offset += max_delta;
		depth++;
	}

	if(depth == 0)
	{
		stats.generate_prefetch.pt_miss[from_ghr]++;
	}
	else
	{
		stats.generate_prefetch.depth += depth;
	}

	// cout << "    [Poff]";
	// for(uint32_t index = 0; index < pref_offsets.size(); ++index)
	// {
	// 	cout << " " << dec << pref_offsets[index];
	// }
	// for(uint32_t index = 0; index < pref_offsets_discarded.size(); ++index)
	// {
	// 	cout << " (" << dec << pref_offsets_discarded[index] << ")";
	// }	
	// if((100*curr_confidence) < knob::spp_max_confidence)
	// {
	// 	cout << " | low_conf " << FIXED_FLOAT((100*curr_confidence));
	// }
	// else if(depth >= knob::spp_max_depth)
	// {
	// 	cout << " | max_depth " << depth;
	// }
	// cout << endl;
	// cout << "    [Pcnf] ";
	// for(uint32_t index = 0; index < conf.size(); ++index)
	// {
	// 	cout << " " << dec << FIXED_FLOAT((100*conf[index]));
	// }
	// for(uint32_t index = 0; index < conf_discarded.size(); ++index)
	// {
	// 	cout << " (" << dec << FIXED_FLOAT((100*conf_discarded[index])) << ")";
	// }
	// cout << endl;
}

double SPP::compute_confidence(double prev_conf, double curr_ratio)
{
	double new_conf = prev_conf * curr_ratio;
	if(knob::spp_enable_alpha)
	{
		new_conf *= alpha;
	}
	return new_conf;
}

int32_t SPP::compute_delta(uint32_t curr_offset, uint32_t prev_offset)
{
	int32_t delta = 0;
	if(curr_offset >= prev_offset)
	{
		delta = (curr_offset - prev_offset);
	}
	else
	{
		delta = (prev_offset - curr_offset);
		delta *= (-1);
	}
	return delta;
}

uint64_t SPP::compute_signature(uint64_t old_sig, int32_t new_delta)
{
	uint64_t new_sig = old_sig;
	new_sig = new_sig << 3;
	new_sig = new_sig & ((1ull << knob::spp_signature_bits) - 1);
	new_delta = new_delta & ((1ul << 7) - 1);
	new_sig = (new_sig ^ new_delta);
	new_sig = new_sig & ((1ull << knob::spp_signature_bits) - 1);	
	return	new_sig;
}

void SPP::dump_stats()
{
	cout << "spp.st.lookup " << stats.st.lookup << endl
		<< "spp.st.hit " << stats.st.hit << endl
		<< "spp.st.insert " << stats.st.insert << endl
		<< "spp.st.evict " << stats.st.evict << endl
		<< "spp.pt.lookup " << stats.pt.lookup << endl
		<< "spp.pt.hit " << stats.pt.hit << endl
		<< "spp.pt.insert " << stats.pt.insert << endl
		<< "spp.pt.evict " << stats.pt.evict << endl
		<< "spp.generate_prefetch.called_PT " << stats.generate_prefetch.called[0] << endl
		<< "spp.generate_prefetch.called_GHR " << stats.generate_prefetch.called[1] << endl
		<< "spp.generate_prefetch.pref_generated_PT " << stats.generate_prefetch.pref_generated[0] << endl
		<< "spp.generate_prefetch.pref_generated_GHR " << stats.generate_prefetch.pref_generated[1] << endl
		<< "spp.generate_prefetch.pt_miss_PT " << stats.generate_prefetch.pt_miss[0] << endl
		<< "spp.generate_prefetch.pt_miss_GHR " << stats.generate_prefetch.pt_miss[1] << endl
		<< "spp.generate_prefetch.avg_depth " << FIXED_FLOAT((double)stats.generate_prefetch.depth/(stats.generate_prefetch.called[0]+stats.generate_prefetch.called[1])) << endl
		<< "spp.pref_filter.lookup_PT " << stats.pref_filter.lookup[0] << endl
		<< "spp.pref_filter.lookup_GHR " << stats.pref_filter.lookup[1] << endl
		<< "spp.pref_filter.hit_PT " << stats.pref_filter.hit[0] << endl
		<< "spp.pref_filter.hit_GHR " << stats.pref_filter.hit[1] << endl
		<< "spp.pref_filter.issue_pref_PT " << stats.pref_filter.issue_pref[0] << endl
		<< "spp.pref_filter.issue_pref_GHR " << stats.pref_filter.issue_pref[1] << endl
		<< "spp.pref_filter.insert_PT " << stats.pref_filter.insert[0] << endl
		<< "spp.pref_filter.insert_GHR " << stats.pref_filter.insert[1] << endl
		<< "spp.pref_filter.evict_PT " << stats.pref_filter.evict[0] << endl
		<< "spp.pref_filter.evict_GHR " << stats.pref_filter.evict[1] << endl
		<< "spp.pref_filter.evict_not_demanded_PT " << stats.pref_filter.evict_not_demanded[0] << endl
		<< "spp.pref_filter.evict_not_demanded_GHR " << stats.pref_filter.evict_not_demanded[1] << endl
		<< "spp.pref_filter.demand_seen_PT " << stats.pref_filter.demand_seen[0] << endl
		<< "spp.pref_filter.demand_seen_GHR " << stats.pref_filter.demand_seen[1] << endl
		<< "spp.pref_filter.demand_seen_unique_PT " << stats.pref_filter.demand_seen_unique[0] << endl
		<< "spp.pref_filter.demand_seen_unique_GHR " << stats.pref_filter.demand_seen_unique[1] << endl
		<< "spp.pref_buffer.buffered " << stats.pref_buffer.buffered << endl
		<< "spp.pref_buffer.spilled " << stats.pref_buffer.spilled << endl
		<< "spp.pref_buffer.issued " << stats.pref_buffer.issued << endl
		<< "spp.ghr.lookup " << stats.ghr.lookup << endl
		<< "spp.ghr.hit " << stats.ghr.hit << endl
		<< "spp.ghr.insert " << stats.ghr.insert << endl
		<< "spp.ghr.evict " << stats.ghr.evict << endl
		<< "spp.alpha.update " << stats.alpha.update << endl
		<< endl;
}

void SPP::filter_prefetch(vector<uint64_t> tmp_pref_addr, vector<uint64_t> &pref_addr, bool from_ghr)
{
	for(uint32_t index = 0; index < tmp_pref_addr.size(); ++index)
	{
		stats.pref_filter.lookup[from_ghr]++;
		auto it = search_prefetch_filter(tmp_pref_addr[index]);
		if(it != prefetch_filter.end())
		{
			stats.pref_filter.hit[from_ghr]++;
		}
		else
		{
			pref_addr.push_back(tmp_pref_addr[index]);
			stats.pref_filter.issue_pref[from_ghr]++;
			insert_prefetch_filter(tmp_pref_addr[index], from_ghr);
			incr_counter(total_pref, knob::spp_max_global_counter_value);
		}
	}
}

deque<PFEntry*>::iterator SPP::search_prefetch_filter(uint64_t address)
{
	return find_if(prefetch_filter.begin(), prefetch_filter.end(), [address](PFEntry *pfentry){return (pfentry->address == address);});
}

void SPP::insert_prefetch_filter(uint64_t address, bool from_ghr)
{
	stats.pref_filter.insert[from_ghr]++;
	if(prefetch_filter.size() >= knob::spp_pf_size)
	{
		auto victim = search_victim_prefetch_filter();
		evict_prefetch_filter(victim);
	}

	PFEntry *pfentry = new PFEntry();
	pfentry->address = address;
	pfentry->demand_hit = false;
	pfentry->from_ghr = from_ghr;
	prefetch_filter.push_back(pfentry);
}

deque<PFEntry*>::iterator SPP::search_victim_prefetch_filter()
{
	return prefetch_filter.begin();
}

void SPP::evict_prefetch_filter(deque<PFEntry*>::iterator victim)
{
	PFEntry *pfentry = (*victim);
	stats.pref_filter.evict[pfentry->from_ghr]++;
	if(!pfentry->demand_hit)
	{
		stats.pref_filter.evict_not_demanded[pfentry->from_ghr]++;
	}
	prefetch_filter.erase(victim);
	delete pfentry;
}

void SPP::register_demand_hit(uint64_t address)
{
	address = (address >> LOG2_BLOCK_SIZE) << LOG2_BLOCK_SIZE;
	auto it = search_prefetch_filter(address);
	if(it != prefetch_filter.end())
	{
		if(!(*it)->demand_hit)
		{
			(*it)->demand_hit = true;
			stats.pref_filter.demand_seen_unique[(*it)->from_ghr]++;
			incr_counter(used_pref, knob::spp_max_global_counter_value);
		}
		stats.pref_filter.demand_seen[(*it)->from_ghr]++;
	}
	demand_counter++;
	if(demand_counter >= knob::spp_alpha_epoch)
	{
		alpha = (double)used_pref/total_pref;
		demand_counter = 0;
		stats.alpha.update++;
	}
}

void SPP::buffer_prefetch(vector<uint64_t> pref_addr)
{
	// cout << "buffering " << pref_addr.size() << " already present " << pref_buffer.size() << endl;
	uint32_t count = 0;
	for(uint32_t index = 0; index < pref_addr.size(); ++index)
	{
		if(pref_buffer.size() >= knob::spp_pref_buffer_size)
		{
			break;
		}
		pref_buffer.push_back(pref_addr[index]);
		count++;
	}
	stats.pref_buffer.buffered += count;
	stats.pref_buffer.spilled += (pref_addr.size() - count);
}

void SPP::issue_prefetch(vector<uint64_t> &pref_addr)
{
	uint32_t count = 0;
	while(!pref_buffer.empty() && count < knob::spp_pref_degree)
	{
		pref_addr.push_back(pref_buffer.front());
		pref_buffer.pop_front();
		count++;
	}
	stats.pref_buffer.issued += pref_addr.size();
}

void SPP::insert_ghr(uint64_t signature, uint32_t offset, int32_t delta, double confidence)
{
	// cout << "sig " << hex << setw(20) << signature
	// 	<< " offset " << dec << setw(2) << offset
	// 	<< " delta " << dec << setw(3) << delta
	// 	<< " confidence " << dec << setw(4) << FIXED_FLOAT(confidence)
	// 	<< endl;

	stats.ghr.insert++;
	GHREntry *ghrentry = NULL;
	if(ghr.size() >= knob::spp_ghr_size)
	{
		auto victim = ghr.begin();
		ghrentry = (*victim);
		ghr.erase(victim);
		delete ghrentry;
		stats.ghr.evict++;
	}

	ghrentry = new GHREntry();
	ghrentry->signature = signature;
	ghrentry->offset = offset;
	ghrentry->delta = delta;
	ghrentry->confidence = confidence;
	ghr.push_back(ghrentry);
}

bool SPP::lookup_ghr(uint32_t offset, uint64_t &signature, double &confidence)
{
	for(uint32_t index = 0; index < ghr.size(); ++index)
	{
		if(compute_congruent_offset(ghr[index]->offset, ghr[index]->delta) == offset)
		{
			signature = ghr[index]->signature;
			confidence = ghr[index]->confidence;
			// cout << "[GHR_hit] offset " << offset << " ghr.offset " << ghr[index]->offset << " ghr.delta " << ghr[index]->delta << endl; 
			return true;
		}
	}
	return false;
}

uint32_t SPP::compute_congruent_offset(uint32_t offset, int32_t delta)
{
	int32_t n_offset = ((int32_t)offset + delta);
	if(n_offset < 0)
	{
		return (uint32_t)(64 - (abs(n_offset) % 64));
	}
	else if(n_offset >= 64)
	{
		return (uint32_t)(n_offset % 64);
	}
	else return (uint32_t)n_offset;
}