#include <iostream>
#include "pref_power7.h"
#include "champsim.h"

namespace knob
{
    extern uint32_t streamer_num_trackers;
    extern uint32_t streamer_pref_degree;
    extern uint32_t stride_num_trackers;
    extern uint32_t stride_pref_degree;
    extern uint32_t power7_explore_epoch;
    extern uint32_t power7_exploit_epoch;
}

void POWER7_Pref::init_knobs()
{
    assert(knob::power7_exploit_epoch >= knob::power7_explore_epoch*Config::NumConfigs);
}

void POWER7_Pref::init_stats()
{

}

void POWER7_Pref::print_config()
{

}

POWER7_Pref::POWER7_Pref(string type, CACHE *cache) : Prefetcher(type), m_parent_cache(cache)
{
    init_knobs();
    init_stats();

    access_counter = 0;
    cycle_stamp = 0;
    clear_cycle_stats();
    mode = Mode::Exploit;
    config = Config::Default;
    set_params();
}

POWER7_Pref::~POWER7_Pref()
{

}

void POWER7_Pref::set_params()
{
    knob::streamer_pref_degree = get_streamer_degree(config);
    knob::stride_pref_degree = get_stride_degree(config);
}

uint32_t POWER7_Pref::get_streamer_degree(Config config)
{
    switch(config)
    {
        case Config::Default:           return 5;
        case Config::Off:               return 0;
        case Config::Shallowest:
        case Config::S_Shallowest:      return 2;
        case Config::Shallow:
        case Config::S_Shallow:         return 3;
        case Config::Medium:
        case Config::S_Medium:          return 4;
        case Config::Deep:
        case Config::S_Deep:            return 5;
        case Config::Deeper:
        case Config::S_Deeper:          return 6;
        case Config::Deepest:
        case Config::S_Deepest:         return 7;
        default:                        return 5;
    }
}

uint32_t POWER7_Pref::get_stride_degree(Config config)
{
    switch(config)
    {
        case Config::Default:           
        case Config::Off:               
        case Config::Shallowest:
        case Config::Shallow:
        case Config::Medium:
        case Config::Deep:
        case Config::Deeper:
        case Config::Deepest:                   return 0;
        case Config::S_Shallowest:
        case Config::S_Shallow:
        case Config::S_Medium:
        case Config::S_Deep:
        case Config::S_Deeper:
        case Config::S_Deepest:                 return 4;
        default:                                return 0;
    }
}

void POWER7_Pref::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
    access_counter++;
    uint64_t cpu_cycle = get_cpu_cycle(m_parent_cache->cpu);

    /* state machine design */
    if(mode == Mode::Exploit)
    {
        /* exploit->explore transition */
        if(access_counter >= knob::power7_exploit_epoch)
        {
            mode = Mode::Explore;
            config = Config::Default;
            set_params();
            access_counter = 0;
            cycle_stamp = cpu_cycle;
            clear_cycle_stats();
        }
    }
    else
    {
        /* exploration config change */
        if(access_counter >= knob::power7_explore_epoch)
        {
            /* if last config, then change to exploitation */
            if(config == Config::S_Deepest)
            {
                cycle_stats[config] = (cpu_cycle - cycle_stamp);
                mode = Mode::Exploit;
                config = get_winner_config();
                clear_cycle_stats();
                set_params();
            }
            else
            {
                cycle_stats[config] = (cpu_cycle - cycle_stamp);
                mode = Mode::Exploit;
                config = (Config)((uint32_t)config + 1); /* try out next config */
                set_params();
            }
            access_counter = 0;
            cycle_stamp = cpu_cycle;
        }
    }

    /* validate */
    assert(mode == Mode::Exploit || mode == Mode::Explore);
    assert(config < Config::NumConfigs);
    if(mode == Mode::Explore)
    {
        for(uint32_t cfg = (uint32_t)Config::Default; config < (uint32_t)config; ++cfg)
        {
            assert(cycle_stats[cfg] != 0);
        }
    }

    /* invoke each individual prefetcher */
    streamer->invoke_prefetcher(pc, address, cache_hit, type, pref_addr);
    stride->invoke_prefetcher(pc, address, cache_hit, type, pref_addr);

    return;
}

bool POWER7_Pref::empty_cycle_stats()
{
    for (uint32_t index = (uint32_t)Config::Default; index < (uint32_t)Config::NumConfigs; ++index)
    {
        if(cycle_stats[index]) return false;
    }
    return true;
}

void POWER7_Pref::clear_cycle_stats()
{
    for(uint32_t index = (uint32_t)Config::Default; index < (uint32_t)Config::NumConfigs; ++index)
    {
        cycle_stats[index] = 0;
    }
}

Config POWER7_Pref::get_winner_config()
{
    uint64_t min_cycles = UINT64_MAX;
    Config min_cfg = Config::Default;

    for (uint32_t index = (uint32_t)Config::Default; index < (uint32_t)Config::NumConfigs; ++index)
    {
        if(cycle_stats[index] <= min_cycles)
        {
            min_cycles = cycle_stats[index];
            min_cfg = (Config)index;
        }
    }

    return min_cfg;
}

void POWER7_Pref::dump_stats()
{

}