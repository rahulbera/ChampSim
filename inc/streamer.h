#ifndef STREAMER_H
#define STREAMER_H

#include <deque>
#include "prefetcher.h"

class ST_Entry
{
public:
    uint64_t page;
    bool dir;
    
public:
    ST_Entry(uint64_t _page, bool _dir) {page=_page; dir=_dir;}
    ~ST_Entry(){}
};

class StreamPrefetcher : public Prefetcher
{
    deque<ST_Entry*>
}