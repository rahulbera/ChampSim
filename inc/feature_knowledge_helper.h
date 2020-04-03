#ifndef FEATURE_KNOWLEDGE_HELPER_H
#define FEATURE_KNOWLEDGE_HELPER_H

#include "util.h"

uint32_t FeatureKnowledge::process_PC(uint64_t pc)
{
	uint32_t raw_index = folded_xor(pc, 2); /* 32-b folded XOR */
	uint32_t hashed_index = HashZoo::getHash(m_hash_type, raw_index);
	return (hashed_index % m_states);
}

uint32_t FeatureKnowledge::process_offset(uint32_t offset)
{
	uint32_t hashed_index = HashZoo::getHash(m_hash_type, offset);
	return (hashed_index % m_states);
}

uint32_t FeatureKnowledge::process_delta(int32_t delta)
{
	uint32_t hashed_index = HashZoo::getHash(m_hash_type, delta);
	return (hashed_index % m_states);
}

uint32_t FeatureKnowledge::process_address(uint64_t address)
{
	uint32_t raw_index = folded_xor(address, 2); /* 32-b folded XOR */
	uint32_t hashed_index = HashZoo::getHash(m_hash_type, raw_index);
	return (hashed_index % m_states);
}

uint32_t FeatureKnowledge::process_PC_offset(uint64_t pc, uint32_t offset)
{
	uint64_t tmp = pc;
	tmp = tmp << 6;
	tmp += offset;
	uint32_t raw_index = folded_xor(tmp, 2);
	uint32_t hashed_index = HashZoo::getHash(m_hash_type, raw_index);
	return (hashed_index % m_states);
}

uint32_t FeatureKnowledge::process_PC_address(uint64_t pc, uint64_t address)
{
	uint64_t tmp = pc;
	tmp = tmp << 16;
	tmp ^= address;
	uint32_t raw_index = folded_xor(tmp, 2);
	uint32_t hashed_index = HashZoo::getHash(m_hash_type, raw_index);
	return (hashed_index % m_states);
}

uint32_t FeatureKnowledge::process_PC_page(uint64_t pc, uint64_t page)
{
	uint64_t tmp = pc;
	tmp = tmp << 16;
	tmp ^= page;
	uint32_t raw_index = folded_xor(tmp, 2);
	uint32_t hashed_index = HashZoo::getHash(m_hash_type, raw_index);
	return (hashed_index % m_states);
}

uint32_t FeatureKnowledge::process_PC_path(uint32_t pc_path)
{
	uint32_t hashed_index = HashZoo::getHash(m_hash_type, pc_path);
	return (hashed_index % m_states);
}

uint32_t FeatureKnowledge::process_delta_path(uint32_t delta_path)
{
	uint32_t hashed_index = HashZoo::getHash(m_hash_type, delta_path);
	return (hashed_index % m_states);
}

uint32_t FeatureKnowledge::process_offset_path(uint32_t offset_path)
{
	uint32_t hashed_index = HashZoo::getHash(m_hash_type, offset_path);
	return (hashed_index % m_states);
}


#endif /* FEATURE_KNOWLEDGE_HELPER_H */

