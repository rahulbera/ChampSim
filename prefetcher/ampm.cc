#include <algorithm>
#include "ampm.h"
#include "champsim.h"

namespace knob
{
    extern uint32_t ampm_pb_size;
    extern uint32_t ampm_pred_degree;
    extern uint32_t ampm_pref_degree;
    extern uint32_t ampm_pref_buffer_size;
}

void AMPM::init_knobs()
{

}

void AMPM::init_stats()
{
    bzero(&stats, sizeof(stats));
}

void AMPM::print_config()
{
    cout << "ampm_pb_size " << knob::ampm_pb_size << endl
         << "ampm_pred_degree " << knob::ampm_pred_degree << endl
         << "ampm_pref_degree " << knob::ampm_pref_degree << endl
         << "ampm_pref_buffer_size " << knob::ampm_pref_buffer_size << endl
         << endl;
}

AMPM::AMPM(string type) : Prefetcher(type)
{
    init_knobs();
    init_stats();
}

void AMPM::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
    uint64_t page = address >> LOG2_PAGE_SIZE;
    uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

    stats.invoke_called++;

    AMPM_PB_Entry *pb_entry = NULL;
    auto it = find_if(page_buffer.begin(), page_buffer.end(), [page](AMPM_PB_Entry *pb_entry){return pb_entry->page_id == page;});
    
    /* page already tracked */
    if(it != page_buffer.end())
    {
        pb_entry = (*it);
        pb_entry->page_id = page;
        pb_entry->bitmap[offset] = true;
        page_buffer.erase(it);
        page_buffer.push_back(pb_entry);
        stats.pb.hit++;
    }
    else
    {
        if(page_buffer.size() >= knob::ampm_pb_size)
        {
            pb_entry = page_buffer.front();
            page_buffer.pop_front();
            delete(pb_entry);
            stats.pb.evict++;
        }

        pb_entry = new AMPM_PB_Entry();
        pb_entry->page_id = page;
        pb_entry->bitmap[offset] = true;
        page_buffer.push_back(pb_entry);
        stats.pb.insert++;
    }

    /* check for +ve deltas */
    vector<int32_t> selected_pos;
    for(int32_t delta = +16; delta >= +1; --delta)
    {
        int32_t one_hop = (offset - 1*delta >= 0) ? (offset - 1*delta) : -1;
        int32_t two_hop = (offset - 2*delta >= 0) ? (offset - 2*delta) : -1;

        if(one_hop >= 0 && two_hop >= 0)
        {
            if(pb_entry->bitmap[one_hop] && pb_entry->bitmap[two_hop])
            {
                selected_pos.push_back(delta);
            }
        }
    }

    /* check for -ve deltas */
    vector<int32_t> selected_neg;
    for (int32_t delta = +16; delta >= +1; --delta)
    {
        int32_t one_hop = (offset + 1*delta < 64) ? (offset + 1*delta) : -1;
        int32_t two_hop = (offset + 2*delta < 64) ? (offset + 2*delta) : -1;

        if (one_hop >= 0 && two_hop >= 0)
        {
            if (pb_entry->bitmap[one_hop] && pb_entry->bitmap[two_hop])
            {
                selected_neg.push_back(delta);
            }
        }
    }

    uint32_t count = 0;
    vector<uint64_t> predicted_addrs;

    /* prioritize posetive delta over negative delta */
    for(uint32_t index = 0; index < selected_pos.size(); ++index)
    {
        if(count >= knob::ampm_pred_degree)
        {
            stats.pred.degree_reached_pos++;
            break;
        }
        assert(selected_pos[index] >= 1 && selected_pos[index] <= 16);
        int32_t pref_offset = offset + selected_pos[index];
        if(pref_offset >= 0 && pref_offset < 64)
        {
            uint64_t pf_addr = (page << LOG2_PAGE_SIZE) + (pref_offset << LOG2_BLOCK_SIZE);
            predicted_addrs.push_back(pf_addr);
            count++;
            stats.pred.pos_histogram[selected_pos[index]]++;
        }
    }
    for (uint32_t index = 0; index < selected_neg.size(); ++index)
    {
        if (count >= knob::ampm_pred_degree)
        {
            stats.pred.degree_reached_neg++;
            break;
        }
        assert(selected_neg[index] >= 1 && selected_neg[index] <= 16);
        int32_t pref_offset = offset - selected_neg[index];
        if (pref_offset >= 0 && pref_offset < 64)
        {
            uint64_t pf_addr = (page << LOG2_PAGE_SIZE) + (pref_offset << LOG2_BLOCK_SIZE);
            predicted_addrs.push_back(pf_addr);
            count++;
            stats.pred.neg_histogram[selected_neg[index]]++;
        }
    }

    stats.pred.total += predicted_addrs.size();

    /* buffer predicted addresses */
    for(uint32_t index = 0; index < predicted_addrs.size(); ++index)
    {
        bool found = false;
        for(uint32_t index2 = 0; index2 < pref_buffer.size(); ++index)
        {
            if(pref_buffer[index2] == predicted_addrs[index])
            {
                found = true;
                stats.pref_buffer.hit++;
                break;
            }
        }

        /* prefetch buffer miss */
        if(!found)
        {
            if(pref_buffer.size() >= knob::ampm_pref_buffer_size)
            {
                /* drops prefetch requests */
                stats.pref_buffer.dropped += (predicted_addrs.size() - index);
                break;
            }
            else
            {
                pref_buffer.push_back(predicted_addrs[index]);
                stats.pref_buffer.insert++;
            }
        }
    }

    /* populate pref_addr */
    for(uint32_t index = 0; index < knob::ampm_pref_degree; ++index)
    {
        pref_addr.push_back(pref_buffer.front());
        pref_buffer.pop_front();
    }

    stats.pref.total += pref_addr.size();
    
    return;
}

void AMPM::dump_stats()
{
    cout << "ampm.invoke_called " << stats.invoke_called << endl
         << "ampm.pb.hit " << stats.pb.hit << endl
         << "ampm.pb.evict " << stats.pb.evict << endl
         << "ampm.pb.insert " << stats.pb.insert << endl
         << "ampm.pred.total " << stats.pred.total << endl
         << "ampm.pred.degree_reached_pos " << stats.pred.degree_reached_pos << endl
         << "ampm.pred.degree_reached_neg " << stats.pred.degree_reached_neg << endl
         << endl;

    assert(stats.pred.pos_histogram[0] == 0);
    assert(stats.pred.neg_histogram[0] == 0);

    for(uint32_t index = 0; index < MAX_OFFSETS; ++index)
    {
        if(stats.pred.pos_histogram[index])
        {
            cout << "ampm.pred.histogram." << index << " " << stats.pred.pos_histogram[index] << endl;
        }
    }
    for (uint32_t index = 0; index < MAX_OFFSETS; ++index)
    {
        if (stats.pred.neg_histogram[index])
        {
            cout << "ampm.pred.histogram.-" << index << " " << stats.pred.neg_histogram[index] << endl;
        }
    }
    cout << endl;

    cout << "ampm.pref_buffer.hit " << stats.pref_buffer.hit << endl
        << "ampm.pref_buffer.dropped " << stats.pref_buffer.dropped << endl
        << "ampm.pref_buffer.insert " << stats.pref_buffer.insert << endl
        << "ampm.pref.total " << stats.pref.total << endl
        << endl;
}