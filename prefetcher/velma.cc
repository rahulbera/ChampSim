#include <iostream>
#include <assert.h>
#include "velma.h"
#include "sms.h"
#include "scooby.h"

namespace knob
{
	extern vector<string> 	velma_candidate_prefetchers;
	extern uint32_t 		velma_pref_selection_type;
}

void Velma::init_knobs()
{

}

void Velma::init_stats()
{
	bzero(&stats, sizeof(stats));
}

Velma::Velma(string type) : Prefetcher(type)
{
	init_knobs();
	init_stats();

	for(uint32_t index = 0; index < knob::velma_candidate_prefetchers.size(); ++index)
	{
		if(!knob::velma_candidate_prefetchers[index].compare("sms"))
		{
			cout << "[Velma] Adding candidate: SMS" << endl;
			SMSPrefetcher *pref_sms = new SMSPrefetcher(knob::velma_candidate_prefetchers[index]);
			m_prefetchers.push_back(pref_sms);
		}
		else if(!knob::velma_candidate_prefetchers[index].compare("scooby"))
		{
			cout << "[Velma] Adding candidate: Scooby" << endl;
			Scooby *pref_scooby = new Scooby(knob::velma_candidate_prefetchers[index]);
			m_prefetchers.push_back(pref_scooby);
		}
		else if(!knob::velma_candidate_prefetchers[index].compare("none"))
		{
			cout << "[Velma] Cannot add prefetcher: none" << endl;
			exit(12);
		}
		else
		{
			cout << "[Velma] Unsupported prefetcher type: " << knob::velma_candidate_prefetchers[index] << endl;
			assert(false);
		}
	}

	assert(m_prefetchers.size() == knob::velma_candidate_prefetchers.size());
}

Velma::~Velma()
{

}

void Velma::print_config()
{
	/* Call print_config of all candidate prefetchers */
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		m_prefetchers[index]->print_config();
	}

	/* TODO: Print own config parameters */
}

void Velma::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
	stats.invoke_prefetcher.called++;
	/* call invoke prefetcher from each individual candidate prefetchers */
	vector<vector<uint64_t> > pref_candidates;
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		vector<uint64_t> pref_addr_local;
		pref_addr_local.clear();
		
		m_prefetchers[index]->invoke_prefetcher(pc, address, cache_hit, type, pref_addr_local);
		pref_candidates.push_back(pref_addr_local);

		stats.invoke_prefetcher.called_dist[index]++;
		stats.invoke_prefetcher.pref_dist[index] += pref_addr_local.size();
	}
	assert(pref_candidates.size() == m_prefetchers.size());

	/* select the final list of prefetches */
	select_prefetch(pref_candidates, pref_addr);
}

void Velma::select_prefetch(vector<vector<uint64_t> > pref_candidates, vector<uint64_t> &pref_addr)
{
	assert(pref_candidates.size() == m_prefetchers.size());

	if(knob::velma_pref_selection_type == 1) /* Select ALL */
	{
		for(uint32_t index = 0; index < pref_candidates.size(); ++index)
		{
			for(uint32_t index2 = 0; index2 < pref_candidates[index].size(); ++index2)
			{
				pref_addr.push_back(pref_candidates[index][index2]);
				stats.issue.pref_dist[index]++;
			}
		}
	}
	else
	{
		cout << "[Velma] Unsupported velma_pref_selection_type " << knob::velma_pref_selection_type << endl;
		assert(false); 
	}
}

void Velma::register_fill(uint64_t address)
{
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		if(!m_prefetchers[index]->get_type().compare("scooby"))
		{
			Scooby *pref_scooby = (Scooby*)m_prefetchers[index];
			pref_scooby->register_fill(address);
		}
	}
}

void Velma::register_prefetch_hit(uint64_t address)
{
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		if(!m_prefetchers[index]->get_type().compare("scooby"))
		{
			Scooby *pref_scooby = (Scooby*)m_prefetchers[index];
			pref_scooby->register_prefetch_hit(address);
		}
	}
}

void Velma::dump_stats()
{
	/* print stats of individual candidate prefetchers first */
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		m_prefetchers[index]->dump_stats();
	}

	/* TODO: print own stats */
	cout << "velma_invoke_prefetcher_called " << stats.invoke_prefetcher.called << endl;
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		cout << "velma_invoke_prefetcher_called_dist_" << m_prefetchers[index]->get_type() << " " << stats.invoke_prefetcher.called_dist[index] << endl;
		cout << "velma_invoke_prefetcher_pref_dist_" << m_prefetchers[index]->get_type() << " " << stats.invoke_prefetcher.pref_dist[index] << endl;
	}

	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		cout << "velma_issue_pref_dist_" << m_prefetchers[index]->get_type() << " " << stats.issue.pref_dist[index] << endl; 
	}

	cout << endl;
}