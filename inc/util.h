#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <string>
#include <sstream>
#include <vector>

uint32_t folded_xor(uint64_t value, uint32_t num_folds);

template <class T> std::string array_to_string(std::vector<T> array, uint32_t size = 0)
{
    std::stringstream ss;
    if (size == 0) size = array.size();
    for(uint32_t index = 0; index < size; ++index)
    {
        ss << array[index] << ",";
    }
    return ss.str();
}

class HashZoo
{
public:
	static uint32_t jenkins(uint32_t value);
	static uint32_t knuth(uint32_t value);
	static uint32_t murmur3(uint32_t value);
	static uint32_t jenkins32(uint32_t key);
	static uint32_t hash32shift(uint32_t key);
	static uint32_t hash32shiftmult(uint32_t key);
	static uint32_t hash64shift(uint32_t key);
	static uint32_t hash5shift(uint32_t key);
	static uint32_t hash7shift(uint32_t key);
	static uint32_t Wang6shift(uint32_t key);
	static uint32_t Wang5shift(uint32_t key);
	static uint32_t Wang4shift( uint32_t key);
	static uint32_t Wang3shift( uint32_t key);
	static uint32_t hybrid1(uint32_t value);
};

#endif /* UTIL_H */

