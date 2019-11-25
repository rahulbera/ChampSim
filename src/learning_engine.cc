#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "learning_engine.h"

const char* PolicyString[] = {"EGreddy"};
const char* MapPolicyString(Policy policy)
{
	assert((uint32_t)policy < Policy::NumPolicies);
	return PolicyString[(uint32_t)policy];
}

const char* LearningTypeString[] = {"QLearning", "SARSA"};
const char* MapLearningTypeString(LearningType type)
{
	assert((uint32_t)type < LearningType::NumLearningTypes);
	return LearningTypeString[(uint32_t)type];
}


LearningEngine::LearningEngine(float alpha, float gamma, float epsilon, uint32_t actions, uint32_t states, uint64_t seed, std::string policy, std::string type)
	: m_alpha(alpha)
	, m_gamma(gamma)
	, m_epsilon(epsilon) // make it small, as true value indicates exploration
	, m_actions(actions)
	, m_states(states)
	, m_seed(seed)
	, m_policy(parsePolicy(policy))
	, m_type(parseLearningType(type))
{
	qtable = (float**)calloc(m_states, sizeof(float*));
	assert(qtable);
	for(uint32_t index = 0; index < m_states; ++index)
	{
		qtable[index] = (float*)calloc(m_actions, sizeof(float));
		assert(qtable);
	}

	/* init Q-table */
	for(uint32_t row = 0; row < m_states; ++row)
	{
		for(uint32_t col = 0; col < m_actions; ++col)
		{
			qtable[row][col] = (float)1ul/(1-gamma);
		}
	}

	generator.seed(m_seed);
	explore = new std::bernoulli_distribution(epsilon);
	actiongen = new std::uniform_int_distribution<int>(0, m_actions-1);
}

LearningEngine::~LearningEngine()
{
	for(uint32_t row = 0; row < m_states; ++row)
	{
		free(qtable[row]);
	}
	free(qtable);
}

LearningType LearningEngine::parseLearningType(std::string str)
{
	if(!str.compare("QLearning"))	return LearningType::QLearning;
	if(!str.compare("SARSA"))		return LearningType::SARSA;

	printf("unsupported learning_type %s\n", str.c_str());
	assert(false);
}

Policy LearningEngine::parsePolicy(std::string str)
{
	if(!str.compare("EGreedy"))		return Policy::EGreedy;

	printf("unsupported policy %s\n", str.c_str());
	assert(false);
}

uint32_t LearningEngine::chooseAction(uint32_t state)
{
	assert(state < m_states);
	uint32_t action = 0;
	if(m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
	{
		if((*explore)(generator))
		{
			action = (*actiongen)(generator); // take random action
		}
		else
		{
			action = getMaxAction(state);
		}
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}
	return action;
}

void LearningEngine::learn(uint32_t state1, uint32_t action1, uint32_t reward, uint32_t state2, uint32_t action2)
{
	if(m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
	{	
		float Qsa1, Qsa2;
		Qsa1 = consultQ(state1, action1);
		Qsa2 = consultQ(state2, action2);
		/* SARSA */
		Qsa1 = Qsa1 + m_alpha * (reward + m_gamma * Qsa2 - Qsa1);
		updateQ(state1, action1, Qsa1);
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}	
}

float LearningEngine::consultQ(uint32_t state, uint32_t action)
{
	assert(state < m_states && action < m_actions);
	float value = qtable[state][action];
	return value;
}

void LearningEngine::updateQ(uint32_t state, uint32_t action, float value)
{
	assert(state < m_states && action < m_actions);
	qtable[state][action] = value;	
}

uint32_t LearningEngine::getMaxAction(uint32_t state)
{
	assert(state < m_states);
	float max = qtable[state][0];
	uint32_t action = 0;
	for(uint32_t index = 1; index < m_actions; ++index)
	{
		if(qtable[state][index] > max)
		{
			max = qtable[state][index];
			action = index;
		}
	}
	return action;
}