#include <iostream>
#include <assert.h>
#include <algorithm>
#include "context_pref.h"
#include "champsim.h"

namespace knob
{
    extern uint32_t cp_cst_size;
    extern uint32_t cp_cst_assoc;
    extern uint32_t cp_max_response_per_cst;
    extern int32_t  cp_init_reward;
    extern uint32_t cp_prefetch_queue_size;
}

void ContextPrefetcher::init_knobs()
{
    cst_num_sets = knob::cp_cst_size / knob::cp_cst_assoc;
}

void ContextPrefetcher::init_stats()
{
    bzero(&stats, sizeof(stats));
}

void ContextPrefetcher::print_config()
{
    cout << "cp_cst_size " << knob::cp_cst_size << endl
        << endl;
}

ContextPrefetcher::ContextPrefetcher(string type) : Prefetcher(type)
{
    init_knobs();
    init_stats();

    /* init CST */
    deque<CST_Entry*> d;
    cst.resize(cst_num_sets, d);
}

ContextPrefetcher::~ContextPrefetcher()
{

}

void ContextPrefetcher::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
    uint64_t page = address >> LOG2_PAGE_SIZE;
    uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

    /* Task 0: get current delta */
    Context *ctx = get_context(pc, address, page, offset);

    /* Task 1: Reward the already prefetched entries */
    assert(prefetch_queue.size() <= knob::cp_prefetch_queue_size);
    for(uint32_t index = 0; index < prefetch_queue.size(); ++index)
    {
        if(prefetch_queue[index].first == address)
        {
            int32_t reward = compute_reward(index); /* recollect: in context prefetcher, reward is a function of position in prefetch queue */
            update_CST(prefetch_queue[index].second, ctx->delta, reward);
        }
    }

    /* Task 2: Update context-demand correlation in CST */
    for(auto it = history_queue.begin(); it != history_queue.end(); ++it)
    {
        insert_CST(it->second, ctx->delta);
    }

    /* Task 3: Generate new prefetches */
    vector<uint64_t> pf_addr;
    generate_prefetch(ctx, address, page, offset, pf_addr);
    assert(pf_addr.size() <= 1);

    for(uint32_t index = 0; index < pf_addr.size(); ++index)
    {
        insert_prefetch_queue(ctx, pf_addr[index]);
        pref_addr.push_back(pf_addr[index]);
    }

    return;
}

Context* ContextPrefetcher::get_context(uint64_t pc, uint64_t address, uint64_t page, uint32_t offset)
{
    /* TODO: implement */
    assert(false);
    return NULL;
}

int32_t ContextPrefetcher::compute_reward(uint32_t pq_index)
{
    /* TODO: implement */
    assert(false);
    return 0;
}

uint32_t ContextPrefetcher::get_context_hash(Context *ctx)
{
    /* TODO: implement */
    assert(false);
    return 0;
}

deque<CST_Entry*>::iterator ContextPrefetcher::search_CST(uint32_t ctx_hash, int32_t &set)
{
    set = ctx_hash % cst_num_sets;
    return find_if(cst[set].begin(), cst[set].end(), [ctx_hash](CST_Entry *entry){return (entry->ctx_hash == ctx_hash);});
}

void ContextPrefetcher::insert_CST(Context *ctx, int32_t delta)
{
    uint32_t ctx_hash = get_context_hash(ctx);
    int32_t set = 0;
    CST_Entry *cst_entry = NULL;
    
    stats.cst.insert.called++;

    auto cst_it = search_CST(ctx_hash, set);
    /* context hit in CST */
    if(cst_it != cst[set].end())
    {
        stats.cst.insert.cst_hit++;
        cst_entry = (*cst_it);
        auto response_it = find_if(cst_entry->responses.begin(), cst_entry->responses.end(), [delta](Response *res) {return res->delta == delta;});

        /* response does not exist */
        if(response_it == cst_entry->responses.end())
        {
            stats.cst.insert.response_miss++;
            if(cst_entry->responses.size() >= knob::cp_max_response_per_cst)
            {
                vector<Response*>::iterator victim = find_victim_response(cst_entry);
                (*victim)->delta = delta;
                (*victim)->reward = knob::cp_init_reward;
                stats.cst.insert.response_evict++;
                stats.cst.insert.response_insert++;
            }
            else
            {
                Response *response = new Response();
                response->delta = delta;
                response->reward = knob::cp_init_reward;
                cst_entry->responses.push_back(response);
                stats.cst.insert.response_insert++;
            }
        }
        /* response already exists */
        else
        {
            stats.cst.insert.response_hit++;
        }
    }
    /* context miss in CST */
    else
    {
        stats.cst.insert.cst_miss++;
        if(cst[set].size() >= knob::cp_cst_assoc)
        {
            auto victim = find_victim_cst(set);
            CST_Entry *victim_entry = (*victim);
            cst[set].erase(victim);
            delete(victim_entry);
            stats.cst.insert.cst_evict++;
        }

        cst_entry = new CST_Entry();
        cst_entry->ctx_hash = ctx_hash;
        Response *response = new Response();
        response->delta = delta;
        response->reward = knob::cp_init_reward;
        cst_entry->responses.push_back(response);
        cst[set].push_front(cst_entry);
        stats.cst.insert.cst_insert++;
    }
}

vector<Response*>::iterator ContextPrefetcher::find_victim_response(CST_Entry *cst_entry)
{
    int32_t min_reward = INT_MAX;
    vector<Response*>::iterator victim;

    for(vector<Response*>::iterator it = cst_entry->responses.begin(); it != cst_entry->responses.end(); ++it)
    {
        if((*it)->reward < min_reward)
        {
            min_reward = (*it)->reward;
            victim = it;
        }
    }

    return victim;
}

deque<CST_Entry*>::iterator ContextPrefetcher::find_victim_cst(int32_t set)
{
    /* recency stack */
    return cst[set].end();
}

void ContextPrefetcher::update_CST(Context *ctx, int32_t delta, int32_t reward)
{
    int32_t set = -1;
    uint32_t ctx_hash = get_context_hash(ctx);

    stats.cst.update.called++;

    auto cst_it = search_CST(ctx_hash, set);
    if(cst_it != cst[set].end())
    {
        stats.cst.update.cst_hit++;
        CST_Entry *cst_entry = (*cst_it);
        auto response_it = find_if(cst_entry->responses.begin(), cst_entry->responses.end(), [delta](Response *res){return res->delta == delta;});
        if(response_it != cst_entry->responses.end())
        {
            Response *response = (*response_it);
            response->reward += reward;
            stats.cst.update.response_hit++;
        }
    }
}

void ContextPrefetcher::generate_prefetch(Context *ctx, uint64_t address, uint64_t page, uint32_t offset, vector<uint64_t> &pref_addr)
{
    int32_t set = -1;
    uint32_t ctx_hash = get_context_hash(ctx);
    auto cst_it = search_CST(ctx_hash, set);
    
    /* return if context not found */
    if(cst_it == cst[set].end())
    {
        return;
    }

    CST_Entry *cst_entry = (*cst_it);
    int32_t max_reward = INT_MIN;
    int32_t max_reward_delta = 0;
        
    /* find the response with max reward */
    for(uint32_t index = 0; index < cst_entry->responses.size(); ++index)
    {
        if(cst_entry->responses[index]->reward >= max_reward)
        {
            max_reward = cst_entry->responses[index]->reward;
            max_reward_delta = cst_entry->responses[index]->delta;
        }
    }

    /* boundary check */
    int32_t predicted_offset = offset + max_reward_delta;
    if(predicted_offset >= 0 && predicted_offset < 64)
    {
        uint64_t p_addr = (page << LOG2_PAGE_SIZE) + (predicted_offset << LOG2_BLOCK_SIZE);
        pref_addr.push_back(p_addr);
    }
}

void ContextPrefetcher::insert_prefetch_queue(Context *ctx, uint64_t address)
{
    if(prefetch_queue.size() >= knob::cp_prefetch_queue_size)
    {
        auto victim_it = prefetch_queue.front();
        delete(victim_it.second);
        // pair<uint64_t, Context*> victim = (*victim_it);
        prefetch_queue.pop_front();
    }

    Context *tmp_ctx = new Context();
    tmp_ctx->copy(ctx);
    prefetch_queue.push_back(pair<uint64_t, Context*>(address, tmp_ctx));
}

void ContextPrefetcher::dump_stats()
{
    /* TODO: implement */
    assert(false);
    return;
}