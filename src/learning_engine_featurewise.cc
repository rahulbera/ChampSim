#include <iostream>
#include <vector>
#include <assert.h>
#include <strings.h>
#include "learning_engine_featurewise.h"
#include "scooby.h"

#if 0
#	define LOCKED(...) {fflush(stdout); __VA_ARGS__; fflush(stdout);}
#	define LOGID() fprintf(stdout, "[%25s@%3u] ", \
							__FUNCTION__, __LINE__ \
							);
#	define MYLOG(...) LOCKED(LOGID(); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n");)
#else
#	define MYLOG(...) {}
#endif

namespace knob
{
	extern vector<int32_t> le_featurewise_active_features;
	extern vector<int32_t> le_featurewise_num_tilings;
	extern vector<int32_t> le_featurewise_num_tiles;
	extern vector<int32_t> le_featurewise_hash_types;
	extern vector<int32_t> le_featurewise_enable_tiling_offset;
}

void LearningEngineFeaturewise::init_knobs()
{
	assert(knob::le_featurewise_active_features.size() == knob::le_featurewise_num_tilings.size());
	assert(knob::le_featurewise_active_features.size() == knob::le_featurewise_num_tiles.size());
	assert(knob::le_featurewise_active_features.size() == knob::le_featurewise_enable_tiling_offset.size());
}

void LearningEngineFeaturewise::init_stats()
{

}

LearningEngineFeaturewise::LearningEngineFeaturewise(Prefetcher *parent, float alpha, float gamma, float epsilon, uint32_t actions, uint64_t seed, std::string policy, std::string type, bool zero_init)
	: LearningEngineBase(parent, alpha, gamma, epsilon, actions, 0 /*dummy state value*/, seed, policy, type)
{
	/* init each feature engine */
	for(uint32_t index = 0; index < NumFeatureTypes; ++index)
	{
		m_feature_knowledges[index] = NULL;
	}
	for(uint32_t index = 0; index < knob::le_featurewise_active_features.size(); ++index)
	{
		assert(knob::le_featurewise_active_features[index] < NumFeatureTypes);
		m_feature_knowledges[knob::le_featurewise_active_features[index]] = new FeatureKnowledge((FeatureType)knob::le_featurewise_active_features[index], 
																							alpha, 
																							gamma, 
																							actions, 
																							knob::le_featurewise_num_tilings[index],
																							knob::le_featurewise_num_tiles[index],
																							zero_init,
																							knob::le_featurewise_hash_types[index],
																							knob::le_featurewise_enable_tiling_offset[index]
																							);
		assert(m_feature_knowledges[knob::le_featurewise_active_features[index]]);
	}

	/* init random generators */
	m_generator.seed(m_seed);
	m_explore = new std::bernoulli_distribution(epsilon);
	m_actiongen = new std::uniform_int_distribution<int>(0, m_actions-1);

	/* init stats */
	bzero(&stats, sizeof(stats));
}

LearningEngineFeaturewise::~LearningEngineFeaturewise()
{
	for(uint32_t index = 0; index < NumFeatureTypes; ++index)
	{
		if(m_feature_knowledges[index])
			delete m_feature_knowledges[index];
	}
}

uint32_t LearningEngineFeaturewise::chooseAction(State *state)
{
	stats.action.called++;
	uint32_t action = 0;
	if(m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
	{
		if((*m_explore)(m_generator))
		{
			action = (*m_actiongen)(m_generator); // take random action
			stats.action.explore++;
			stats.action.dist[action][0]++;
			MYLOG("action taken %u explore, state %s, scores %s", action, state->to_string().c_str(), getStringQ(state).c_str());
		}
		else
		{
			action = getMaxAction(state);
			stats.action.exploit++;
			stats.action.dist[action][1]++;
			MYLOG("action taken %u exploit, state %s, scores %s", action, state->to_string().c_str(), getStringQ(state).c_str());
		}
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}

	return action;
}

void LearningEngineFeaturewise::learn(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2)
{
	stats.learn.called++;
	if(m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
	{	
		for(uint32_t index = 0; index < NumFeatureTypes; ++index)
		{
			if(m_feature_knowledges[index])
			{
				m_feature_knowledges[index]->updateQ(state1, action1, reward, state2, action2);
			}
		}
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}
}

uint32_t LearningEngineFeaturewise::getMaxAction(State *state)
{
	float max_q_value = 0.0, q_value = 0.0;
	uint32_t selected_action = 0;

	for(uint32_t action = 0; action < m_actions; ++action)
	{
		q_value = consultQ(state, action);
		if(q_value > max_q_value)
		{
			max_q_value = q_value;
			selected_action = action;
		}
	}
	return selected_action;
}

float LearningEngineFeaturewise::consultQ(State *state, uint32_t action)
{
	assert(action < m_actions);
	float q_value = 0.0;

	/* accumulate Q-value accross all feature tables */
	for(uint32_t index = 0; index < NumFeatureTypes; ++index)
	{
		if(m_feature_knowledges[index])
		{
			q_value += m_feature_knowledges[index]->retrieveQ(state, action);
		}
	}
	return q_value;
}

void LearningEngineFeaturewise::dump_stats()
{
	Scooby *scooby = (Scooby*)m_parent;
	fprintf(stdout, "learning_engine.action.called %lu\n", stats.action.called);
	fprintf(stdout, "learning_engine.action.explore %lu\n", stats.action.explore);
	fprintf(stdout, "learning_engine.action.exploit %lu\n", stats.action.exploit);
	for(uint32_t action = 0; action < m_actions; ++action)
	{
		fprintf(stdout, "learning_engine.action.index_%d_explored %lu\n", scooby->getAction(action), stats.action.dist[action][0]);
		fprintf(stdout, "learning_engine.action.index_%d_exploited %lu\n", scooby->getAction(action), stats.action.dist[action][1]);
	}
	fprintf(stdout, "learning_engine.learn.called %lu\n", stats.learn.called);
	fprintf(stdout, "\n");
}