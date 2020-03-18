/* Based on Multi-Lookahead Offset Prefetcher (MLOP) - 3rd Data Prefetching Championship */
/* Owners: Mehran Shakerinava and Mohammad Bakhshalipour */


#ifndef MLOP_H
#define MLOP_H

#include <bits/stdc++.h>
#include "prefetcher.h"
#include "cache.h"

/**
 * A class for printing beautiful data tables.
 * It's useful for logging the information contained in tabular structures.
 */
class Table {
  public:
    Table(int width, int height) : width(width), height(height), cells(height, vector<string>(width)) {}

    void set_row(int row, const vector<string> &data, int start_col = 0) {
        // assert(data.size() + start_col == this->width);
        for (unsigned col = start_col; col < this->width; col += 1)
            this->set_cell(row, col, data[col]);
    }

    void set_col(int col, const vector<string> &data, int start_row = 0) {
        // assert(data.size() + start_row == this->height);
        for (unsigned row = start_row; row < this->height; row += 1)
            this->set_cell(row, col, data[row]);
    }

    void set_cell(int row, int col, string data) {
        // assert(0 <= row && row < (int)this->height);
        // assert(0 <= col && col < (int)this->width);
        this->cells[row][col] = data;
    }

    void set_cell(int row, int col, double data) {
        ostringstream oss;
        oss << setw(11) << fixed << setprecision(8) << data;
        this->set_cell(row, col, oss.str());
    }

    void set_cell(int row, int col, int64_t data) {
        ostringstream oss;
        oss << setw(11) << std::left << data;
        this->set_cell(row, col, oss.str());
    }

    void set_cell(int row, int col, int data) { this->set_cell(row, col, (int64_t)data); }

    void set_cell(int row, int col, uint64_t data) {
        ostringstream oss;
        oss << "0x" << setfill('0') << setw(16) << hex << data;
        this->set_cell(row, col, oss.str());
    }

    /**
     * @return The entire table as a string
     */
    string to_string() {
        vector<int> widths;
        for (unsigned i = 0; i < this->width; i += 1) {
            int max_width = 0;
            for (unsigned j = 0; j < this->height; j += 1)
                max_width = max(max_width, (int)this->cells[j][i].size());
            widths.push_back(max_width + 2);
        }
        string out;
        out += Table::top_line(widths);
        out += this->data_row(0, widths);
        for (unsigned i = 1; i < this->height; i += 1) {
            out += Table::mid_line(widths);
            out += this->data_row(i, widths);
        }
        out += Table::bot_line(widths);
        return out;
    }

    string data_row(int row, const vector<int> &widths) {
        string out;
        for (unsigned i = 0; i < this->width; i += 1) {
            string data = this->cells[row][i];
            data.resize(widths[i] - 2, ' ');
            out += " | " + data;
        }
        out += " |\n";
        return out;
    }

    static string top_line(const vector<int> &widths) { return Table::line(widths, "┌", "┬", "┐"); }

    static string mid_line(const vector<int> &widths) { return Table::line(widths, "├", "┼", "┤"); }

    static string bot_line(const vector<int> &widths) { return Table::line(widths, "└", "┴", "┘"); }

    static string line(const vector<int> &widths, string left, string mid, string right) {
        string out = " " + left;
        for (unsigned i = 0; i < widths.size(); i += 1) {
            int w = widths[i];
            for (int j = 0; j < w; j += 1)
                out += "─";
            if (i != widths.size() - 1)
                out += mid;
            else
                out += right;
        }
        return out + "\n";
    }

  private:
    unsigned width;
    unsigned height;
    vector<vector<string>> cells;
};

template <class T> class SetAssociativeCache {
  public:
    class Entry {
      public:
        uint64_t key;
        uint64_t index;
        uint64_t tag;
        bool valid;
        T data;
    };

    SetAssociativeCache(int size, int num_ways, int debug_level = 0)
        : size(size), num_ways(num_ways), num_sets(size / num_ways), entries(num_sets, vector<Entry>(num_ways)),
          cams(num_sets), debug_level(debug_level) {
        // assert(size % num_ways == 0);
        for (int i = 0; i < num_sets; i += 1)
            for (int j = 0; j < num_ways; j += 1)
                entries[i][j].valid = false;
        /* calculate `index_len` (number of bits required to store the index) */
        for (int max_index = num_sets - 1; max_index > 0; max_index >>= 1)
            this->index_len += 1;
    }

    /**
     * Invalidates the entry corresponding to the given key.
     * @return A pointer to the invalidated entry
     */
    Entry *erase(uint64_t key) {
        Entry *entry = this->find(key);
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        auto &cam = cams[index];
        int num_erased = cam.erase(tag);
        if (entry)
            entry->valid = false;
        // assert(entry ? num_erased == 1 : num_erased == 0);
        return entry;
    }

    /**
     * @return The old state of the entry that was updated
     */
    Entry insert(uint64_t key, const T &data) {
        Entry *entry = this->find(key);
        if (entry != nullptr) {
            Entry old_entry = *entry;
            entry->data = data;
            return old_entry;
        }
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        vector<Entry> &set = this->entries[index];
        int victim_way = -1;
        for (int i = 0; i < this->num_ways; i += 1)
            if (!set[i].valid) {
                victim_way = i;
                break;
            }
        if (victim_way == -1) {
            victim_way = this->select_victim(index);
        }
        Entry &victim = set[victim_way];
        Entry old_entry = victim;
        victim = {key, index, tag, true, data};
        auto &cam = cams[index];
        if (old_entry.valid) {
            int num_erased = cam.erase(old_entry.tag);
            // assert(num_erased == 1);
        }
        cam[tag] = victim_way;
        return old_entry;
    }

    Entry *find(uint64_t key) {
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        auto &cam = cams[index];
        if (cam.find(tag) == cam.end())
            return nullptr;
        int way = cam[tag];
        Entry &entry = this->entries[index][way];
        // assert(entry.tag == tag && entry.valid);
        return &entry;
    }

    /**
     * Creates a table with the given headers and populates the rows by calling `write_data` on all
     * valid entries contained in the cache. This function makes it easy to visualize the contents
     * of a cache.
     * @return The constructed table as a string
     */
    string log(vector<string> headers) {
        vector<Entry> valid_entries = this->get_valid_entries();
        Table table(headers.size(), valid_entries.size() + 1);
        table.set_row(0, headers);
        for (unsigned i = 0; i < valid_entries.size(); i += 1)
            this->write_data(valid_entries[i], table, i + 1);
        return table.to_string();
    }

    int get_index_len() { return this->index_len; }

    void set_debug_level(int debug_level) { this->debug_level = debug_level; }

  protected:
    /* should be overriden in children */
    virtual void write_data(Entry &entry, Table &table, int row) {}

    /**
     * @return The way of the selected victim
     */
    virtual int select_victim(uint64_t index) {
        /* random eviction policy if not overriden */
        return rand() % this->num_ways;
    }

    vector<Entry> get_valid_entries() {
        vector<Entry> valid_entries;
        for (int i = 0; i < num_sets; i += 1)
            for (int j = 0; j < num_ways; j += 1)
                if (entries[i][j].valid)
                    valid_entries.push_back(entries[i][j]);
        return valid_entries;
    }

    int size;
    int num_ways;
    int num_sets;
    int index_len = 0; /* in bits */
    vector<vector<Entry>> entries;
    vector<unordered_map<uint64_t, int>> cams;
    int debug_level = 0;
};

template <class T> class LRUSetAssociativeCache : public SetAssociativeCache<T> {
    typedef SetAssociativeCache<T> Super;

  public:
    LRUSetAssociativeCache(int size, int num_ways, int debug_level = 0)
        : Super(size, num_ways, debug_level), lru(this->num_sets, vector<uint64_t>(num_ways)) {}

    void set_mru(uint64_t key) { *this->get_lru(key) = this->t++; }

    void set_lru(uint64_t key) { *this->get_lru(key) = 0; }

  protected:
    /* @override */
    int select_victim(uint64_t index) {
        vector<uint64_t> &lru_set = this->lru[index];
        return min_element(lru_set.begin(), lru_set.end()) - lru_set.begin();
    }

    uint64_t *get_lru(uint64_t key) {
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        // assert(this->cams[index].count(tag) == 1);
        int way = this->cams[index][tag];
        return &this->lru[index][way];
    }

    vector<vector<uint64_t>> lru;
    uint64_t t = 1;
};

uint64_t hash_index(uint64_t key, int index_len);

/*=== End Of Cache Framework ===*/

/**
 * The access map table records blocks as being in one of 3 general states:
 * ACCESS, PREFETCH, or INIT.
 * The PREFETCH state is actually composed of up to 3 sub-states:
 * L1-PREFETCH, L2-PREFETCH, or L3-PREFETCH.
 * This version of MLOP does not prefetch into L3 so there are 4 states in total (2-bit states).
 */
enum MLOP_State { INIT = 0, ACCESS = 1, PREFTCH = 2 };
char getStateChar(MLOP_State state);
string map_to_string(const vector<MLOP_State> &access_map, const vector<int> &prefetch_map);

class AccessMapData {
  public:
    /* block states are represented with a `MLOP_State` and an `int` in this software implementation but
     * in a hardware implementation, they'd be represented with only 2 bits. */
    vector<MLOP_State> access_map;
    vector<int> prefetch_map;

    deque<int> hist_queue;
};

class AccessMapTable : public LRUSetAssociativeCache<AccessMapData> {
    typedef LRUSetAssociativeCache<AccessMapData> Super;

  public:
    /* NOTE: zones are equivalent to pages (64 blocks) in this implementation */
    AccessMapTable(int size, int blocks_in_zone, int queue_size, int debug_level = 0, int num_ways = 16)
        : Super(size, num_ways, debug_level), blocks_in_zone(blocks_in_zone), queue_size(queue_size) {
        if (this->debug_level >= 1)
            cout << "AccessMapTable::AccessMapTable(size=" << size << ", blocks_in_zone=" << blocks_in_zone
                 << ", queue_size=" << queue_size << ", debug_level=" << debug_level << ", num_ways=" << num_ways << ")"
                 << endl;
    }

    /**
     * Sets specified block to given state. If new state is ACCESS, the block will also be pushed in the zone's queue.
     */
    void set_state(uint64_t block_number, MLOP_State new_state, int new_fill_level = 0) {
        if (this->debug_level >= 2)
            cout << "AccessMapTable::set_state(block_number=0x" << hex << block_number
                 << ", new_state=" << getStateChar(new_state) << ", new_fill_level=" << new_fill_level << ")" << dec
                 << endl;

        // if (new_state != MLOP_State::PREFTCH)
        //     assert(new_fill_level == 0);
        // else
        //     assert(new_fill_level == FILL_L1 || new_fill_level == FILL_L2 || new_fill_level == FILL_LLC);

        uint64_t zone_number = block_number / this->blocks_in_zone;
        int zone_offset = block_number % this->blocks_in_zone;

        uint64_t key = this->build_key(zone_number);
        Entry *entry = Super::find(key);
        if (!entry) {
            // assert(new_state != MLOP_State::PREFTCH);
            if (new_state == MLOP_State::INIT)
                return;
            Super::insert(key, {vector<MLOP_State>(blocks_in_zone, MLOP_State::INIT), vector<int>(blocks_in_zone, 0)});
            entry = Super::find(key);
            // assert(entry->data.hist_queue.empty());
        }

        auto &access_map = entry->data.access_map;
        auto &prefetch_map = entry->data.prefetch_map;
        auto &hist_queue = entry->data.hist_queue;

        if (new_state == MLOP_State::ACCESS) {
            Super::set_mru(key);

            /* insert access into queue */
            hist_queue.push_front(zone_offset);
            if (hist_queue.size() > this->queue_size)
                hist_queue.pop_back();
        }

        MLOP_State old_state = access_map[zone_offset];
        int old_fill_level = prefetch_map[zone_offset];

        vector<MLOP_State> old_access_map = access_map;
        vector<int> old_prefetch_map = prefetch_map;

        access_map[zone_offset] = new_state;
        prefetch_map[zone_offset] = new_fill_level;

        if (new_state == MLOP_State::INIT) {
            /* delete entry if access map is empty (all in state INIT) */
            bool all_init = true;
            for (unsigned i = 0; i < this->blocks_in_zone; i += 1)
                if (access_map[i] != MLOP_State::INIT) {
                    all_init = false;
                    break;
                }
            if (all_init)
                Super::erase(key);
        }

        if (this->debug_level >= 2) {
            cout << "[AccessMapTable::set_state] zone_number=0x" << hex << zone_number << dec
                 << ", zone_offset=" << setw(2) << zone_offset << ": state transition from " << getStateChar(old_state)
                 << " to " << getStateChar(new_state) << endl;
            if (old_state != new_state || old_fill_level != new_fill_level) {
                cout << "[AccessMapTable::set_state] old_access_map=" << map_to_string(old_access_map, old_prefetch_map)
                     << endl;
                cout << "[AccessMapTable::set_state] new_access_map=" << map_to_string(access_map, prefetch_map)
                     << endl;
            }
        }
    }

    Entry *find(uint64_t zone_number) {
        if (this->debug_level >= 2)
            cout << "AccessMapTable::find(zone_number=0x" << hex << zone_number << ")" << dec << endl;
        uint64_t key = this->build_key(zone_number);
        return Super::find(key);
    }

    string log() {
        vector<string> headers({"Zone", "Access Map"});
        return Super::log(headers);
    }

  private:
    /* @override */
    void write_data(Entry &entry, Table &table, int row) {
        uint64_t zone_number = hash_index(entry.key, this->index_len);
        table.set_cell(row, 0, zone_number);
        table.set_cell(row, 1, map_to_string(entry.data.access_map, entry.data.prefetch_map));
    }

    uint64_t build_key(uint64_t zone_number) {
        uint64_t key = zone_number; /* no truncation (52 bits) */
        return hash_index(key, this->index_len);
    }

    unsigned blocks_in_zone;
    unsigned queue_size;

    /*===================================================================*/
    /* Entry   = [tag, map, queue, valid, LRU]                           */
    /* Storage = size * (52 - lg(sets) + 64 * 2 + 15 * 6 + 1 + lg(ways)) */
    /* L1D: 256 * (52 - lg(16) + 128 + 90 + 1 + lg(16)) = 8672 Bytes     */
    /*===================================================================*/
};

template <class T> inline T square(T x) { return x * x; }

class MLOP : public Prefetcher
{
private:
    inline bool is_inside_zone(uint32_t zone_offset) { return (0 <= zone_offset && zone_offset < this->blocks_in_zone); }

	uint32_t PF_DEGREE;
	uint32_t NUM_UPDATES;
	uint32_t L1D_THRESH;
	uint32_t L2C_THRESH;
	uint32_t LLC_THRESH;
	uint32_t ORIGIN;
	uint32_t NUM_OFFSETS;
    CACHE *parent; 

	int32_t MAX_OFFSET, MIN_OFFSET;
    uint32_t blocks_in_cache, blocks_in_zone, amt_size;
    AccessMapTable *access_map_table;

    /**
     * Contains best offsets for each degree of prefetching. A degree will have several offsets if
     * they all had maximum score (thus, a vector of vectors). A degree won't have any offsets if
     * all best offsets were redundant (already selected in previous degrees).
     */
    vector<vector<int>> pf_offset;

    vector<vector<int>> offset_scores;
    vector<int> pf_level; /* the prefetching level for each degree of prefetching offsets */
    uint32_t update_cnt = 0;  /* tracks the number of score updates, round is over when `update_cnt == NUM_UPDATES` */

    uint32_t debug_level = 0;

    /* stats */
    const uint64_t TRACKED_ZONE_CNT = 100;
    bool tracking = false;
    uint64_t tracked_zone_number = 0;
    uint64_t zone_cnt = 0;
    vector<string> zone_life;

    uint64_t round_cnt = 0;
    uint64_t pf_degree_sum = 0, pf_degree_sqr_sum = 0;
    uint64_t max_score_le_sum = 0, max_score_le_sqr_sum = 0;
    uint64_t max_score_ri_sum = 0, max_score_ri_sqr_sum = 0;

private:
	void init_knobs();
	void init_stats();

public:
	MLOP(string type, CACHE *cache);
	~MLOP();
	void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, std::vector<uint64_t> &pref_addr);
	void register_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr);
	void dump_stats();
	void print_config();

	void access(uint64_t block_number);
	void prefetch(CACHE *cache, uint64_t block_number);
    void mark(uint64_t block_number, MLOP_State state, int fill_level = 0);
    void set_debug_level(int debug_level);
    string log_offset_scores();
    void log();
    void track(uint64_t block_number);
    void reset_stats();
    void print_stats();
};

# endif /* MLOP_H */

