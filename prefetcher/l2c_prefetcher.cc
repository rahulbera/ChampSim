#include <string>
#include "cache.h"
#include "prefetcher.h"
#include "sms.h"
#include "spp.h"
#include "scooby.h"

using namespace std;

namespace knob
{
	extern string l2c_prefetcher_type;
}

vector<Prefetcher*> prefetchers;

void CACHE::l2c_prefetcher_initialize() 
{
	if(!knob::l2c_prefetcher_type.compare("none"))
	{
		cout << "adding L2C_PREFETCHER: NONE" << endl;
	}
	else if(!knob::l2c_prefetcher_type.compare("sms"))
	{
		cout << "adding L2C_PREFETCHER: SMS" << endl;
		SMSPrefetcher *pref_sms = new SMSPrefetcher(knob::l2c_prefetcher_type);
		prefetchers.push_back(pref_sms);
	}
	else if(!knob::l2c_prefetcher_type.compare("spp"))
	{
		cout << "adding L2C_PREFETCHER: SPP" << endl;
		SPP *pref_spp = new SPP(knob::l2c_prefetcher_type);
		prefetchers.push_back(pref_spp);
	}

	else if(!knob::l2c_prefetcher_type.compare("scooby"))
	{
		cout << "adding L2C_PREFETCHER: Scooby" << endl;
		Scooby *pref_scooby = new Scooby(knob::l2c_prefetcher_type);
		prefetchers.push_back(pref_scooby);
	}
	else
	{
		cout << "unsupported prefetcher type " << knob::l2c_prefetcher_type << endl;
		exit(1);
	}
}

uint32_t CACHE::l2c_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in)
{
	vector<uint64_t> pref_addr;
	for(uint32_t index = 0; index < prefetchers.size(); ++index)
	{
			prefetchers[index]->invoke_prefetcher(ip, addr, pref_addr);
			if(!pref_addr.empty())
			{
				for(uint32_t addr_index = 0; addr_index < pref_addr.size(); ++addr_index)
				{
					prefetch_line(ip, addr, pref_addr[addr_index], FILL_L2, 0);
				}
			}
			pref_addr.clear();
	}

	return metadata_in;
}

uint32_t CACHE::l2c_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
	if(prefetch)
	{
		for(uint32_t index = 0; index < prefetchers.size(); ++index)
		{
			if(!prefetchers[index]->get_type().compare("scooby"))
			{
				Scooby *pref_scooby = (Scooby*)prefetchers[index];
				pref_scooby->register_fill(addr);
			}
		}
	}

	return metadata_in;
}

void CACHE::l2c_prefetcher_final_stats()
{
	for(uint32_t index = 0; index < prefetchers.size(); ++index)
	{
		prefetchers[index]->dump_stats();
	}	
}

void CACHE::l2c_prefetcher_print_config()
{
	for(uint32_t index = 0; index < prefetchers.size(); ++index)
	{
		prefetchers[index]->print_config();
	}		
}
