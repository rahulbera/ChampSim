/*
 * Closely modeled after Leoor's work on RL-based context prefetching, ISCA'15
 * https://yoav.net.technion.ac.il/files/2016/05/Context-ISCA2015.pdf
 */

#ifndef CONTEXT_PREF_H
#define CONTEXT_PREF_H

#include <vector>
#include <deque>
#include "prefetcher.h"

using namespace std;

class Context
{
public:
    uint64_t pc;
    uint64_t page_id;
    int32_t delta;

    Context()
    {
        pc = 0;
        page_id = 0;
        delta = 0;
    }
    ~Context(){}
    void copy(Context *ctx)
    {
        pc = ctx->pc;
        page_id = ctx->page_id;
        delta = ctx->delta;
    }
};

class Response
{
public:
    int32_t delta;
    int32_t reward;

    Response() {delta = 0; reward = 0;}
    ~Response() {}
};

class PB_Entry
{
public:
    uint64_t page_id;
    uint64_t last_offset;
    uint64_t last_delta;
    deque<uint64_t> pcs;
    deque<uint32_t> offsets;

    PB_Entry()
    {
        page_id = 0xdeadbeef;
        last_offset = 0;
        last_delta = 0;
        pcs.clear();
        offsets.clear();
    }
    ~PB_Entry(){}
};

class CST_Entry
{
public:
    uint32_t ctx_hash;
    vector<Response*> responses;
    uint32_t age;
    
    CST_Entry()
    {
        ctx_hash = 0;
        age = 0;
        responses.clear();
    }
    ~CST_Entry() {}
};

class ContextPrefetcher : public Prefetcher
{
private:
    vector<deque<CST_Entry*> > cst;
    deque<pair<uint64_t, Context*> > history_queue;
    deque<pair<uint64_t, Context*> > prefetch_queue;
    deque<PB_Entry*> page_buffer;

    uint32_t cst_num_sets;

    /* stats */
    struct
    {
        struct
        {
            struct
            {
                uint64_t called;
                uint64_t cst_hit;
                uint64_t response_miss;
                uint64_t response_evict;
                uint64_t response_insert;
                uint64_t response_hit;
                uint64_t cst_miss;
                uint64_t cst_evict;
                uint64_t cst_insert;
            } insert;

            struct
            {
                uint64_t called;
                uint64_t cst_hit;
                uint64_t response_hit;
            } update;

        } cst;
        
    } stats;

private:
    void init_knobs();
    void init_stats();
    deque<CST_Entry *>::iterator search_CST(uint32_t ctx_hash, int32_t &set);
    void generate_prefetch(Context *ctx, uint64_t address, uint64_t page, uint32_t offset, vector<uint64_t> &pref_addr);
    void insert_CST(Context *ctx, int32_t delta);
    vector<Response *>::iterator find_victim_response(CST_Entry *cst_entry);
    deque<CST_Entry *>::iterator find_victim_cst(int32_t set);
    void update_CST(Context *ctx, int32_t delta, int32_t reward);
    void insert_prefetch_queue(Context *ctx, uint64_t address);
    uint32_t get_context_hash(Context *ctx);
    int32_t compute_reward(uint32_t pq_index);
    Context* get_context(uint64_t pc, uint64_t address, uint64_t page, uint32_t offset);

        public : ContextPrefetcher(string type);
    ~ContextPrefetcher();
    void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);
    void dump_stats();
    void print_config();
};


#endif
