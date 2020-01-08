#include <assert.h>
#include <algorithm>
#include "shaggy.h"
#include "champsim.h"

namespace knob
{
	extern uint32_t shaggy_pb_size;
	extern uint32_t shaggy_st_size;
	extern uint32_t shaggy_degree;
	extern uint32_t shaggy_sig_length;
	extern uint32_t shaggy_sig_type;
	extern uint32_t shaggy_page_access_threshold;
}

void Shaggy::init_knobs()
{

}

void Shaggy::init_stats()
{
	bzero(&stats, sizeof(stats));
}

void Shaggy::print_config()
{
	cout << "shaggy_pb_size " << knob::shaggy_pb_size << endl
		<< "shaggy_st_size " << knob::shaggy_st_size << endl
		<< "shaggy_degree " << knob::shaggy_degree << endl
		<< "shaggy_sig_length " << knob::shaggy_sig_length << endl
		<< "shaggy_sig_type " << knob::shaggy_sig_type << endl
		;
}

Shaggy::Shaggy()
{
	init_knobs();
	init_stats();
}

Shaggy::~Shaggy()
{

}

void Shaggy::predict(uint64_t page, uint32_t offset, vector<uint64_t> &pref_addr)
{
	stats.predict.called++;
	Shaggy_PBEntry *pbentry = search_pb(page);
	if(!pbentry)
	{
		if(prefetch_buffer.size() >= knob::shaggy_pb_size)
		{
			stats.predict.pb_evict++;
			prefetch_buffer.pop_front();
		}

		stats.predict.pb_insert++;
		pbentry = new Shaggy_PBEntry(page);
		prefetch_buffer.push_back(pbentry);
	}

	uint32_t last_prefetched_offset = pbentry->last_prefetched_offset;
	uint32_t pref_offset = 0;
	for(uint32_t index = 0; index < knob::shaggy_degree; ++index)
	{
		pref_offset = last_prefetched_offset + 1;
		if(pref_offset >= 64)
		{
			break;
		}
		uint64_t addr = (page << LOG2_PAGE_SIZE) + (pref_offset << LOG2_BLOCK_SIZE);
		pref_addr.push_back(addr);
		last_prefetched_offset = pref_offset;
	}
	pbentry->last_prefetched_offset = pref_offset;
	stats.predict.predicted += pref_addr.size();
}

Shaggy_PBEntry* Shaggy::search_pb(uint64_t page)
{
	auto it = find_if(prefetch_buffer.begin(), prefetch_buffer.end(), [page](Shaggy_PBEntry *pbentry){return pbentry->page == page;});
	return it != prefetch_buffer.end() ? (*it) : NULL;
}

void Shaggy::record_signature(Scooby_STEntry *stentry)
{
	stats.record_signature.called++;
	if (BitmapHelper::count_bits_set(stentry->bmp_real) < knob::shaggy_page_access_threshold)
	{
		stats.record_signature.cond_not_met++;
		return;
	}

	uint64_t signature = create_signature(stentry->trigger_pc, stentry->trigger_offset);
	if(signature_table.size() >= knob::shaggy_st_size)
	{
		stats.record_signature.st_evict++;
		signature_table.pop_front();
	}
	stats.record_signature.st_insert++;
	signature_table.push_back(signature);
}

bool Shaggy::lookup_signature(uint64_t pc, uint64_t page, uint32_t offset)
{
	uint64_t signature = create_signature(pc, offset);
	for(uint32_t index = 0; index < signature_table.size(); ++index)
	{
		if(signature_table[index] == signature)
		{
			return true;
		}
	}
	return false;
}

uint64_t Shaggy::create_signature(uint64_t pc, uint32_t offset)
{
	switch(knob::shaggy_sig_type)
	{
		case 1:
			return (pc % knob::shaggy_sig_length);
		case 2:
			return (((pc << 6) + offset) % knob::shaggy_sig_length);
		default:
			assert(false);
	}
}

void Shaggy::dump_stats()
{
	cout << "shaggy_predict_called "<< stats.predict.called << endl
		<< "shaggy_predict_pb_evict "<< stats.predict.pb_evict << endl
		<< "shaggy_predict_pb_insert "<< stats.predict.pb_insert << endl
		<< "shaggy_predict_predicted "<< stats.predict.predicted << endl
		<< "shaggy_record_signature_called "<< stats.record_signature.called << endl
		<< "shaggy_record_signature_cond_not_met "<< stats.record_signature.cond_not_met << endl
		<< "shaggy_record_signature_st_evict "<< stats.record_signature.st_evict << endl
		<< "shaggy_record_signature_st_insert "<< stats.record_signature.st_insert << endl
		<< endl;
}
