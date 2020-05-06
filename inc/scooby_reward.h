#ifndef SCOOBY_REWARD_H
#define SCOOBY_REWARD_H

namespace knob
{
	extern bool     scooby_enable_het_reward;
	extern bool     scooby_enable_hbw_reward;

	extern int32_t  scooby_reward_none;
	extern int32_t  scooby_reward_incorrect_low_cache_acc;
	extern int32_t  scooby_reward_incorrect_high_cache_acc;
	extern int32_t  scooby_reward_correct_untimely;
	extern int32_t  scooby_reward_correct_timely;
	extern int32_t  scooby_reward_out_of_bounds;
	extern int32_t  scooby_reward_tracker_hit;

	extern int32_t  scooby_reward_fa_none;
	extern int32_t  scooby_reward_fa_incorrect_low_cache_acc;
	extern int32_t  scooby_reward_fa_incorrect_high_cache_acc;
	extern int32_t  scooby_reward_fa_correct_timely;
	extern int32_t  scooby_reward_fa_correct_untimely;
	extern int32_t  scooby_reward_fa_out_of_bounds;
	extern int32_t  scooby_reward_fa_tracker_hit;

	extern int32_t  scooby_reward_hbw_none;
	extern int32_t  scooby_reward_hbw_incorrect_low_cache_acc;
	extern int32_t  scooby_reward_hbw_incorrect_high_cache_acc;
	extern int32_t  scooby_reward_hbw_correct_timely;
	extern int32_t  scooby_reward_hbw_correct_untimely;
	extern int32_t  scooby_reward_hbw_out_of_bounds;
	extern int32_t  scooby_reward_hbw_tracker_hit;
}


int32_t Scooby::compute_reward(Scooby_PTEntry *ptentry, RewardType type)
{
	bool is_first_action = (knob::scooby_enable_het_reward && ptentry->action_index == 0) ? true : false;
	bool high_bw = (knob::scooby_enable_hbw_reward && is_high_bw()) ? true : false;
	int32_t reward = 0;

	stats.reward.compute_reward.dist[type][high_bw]++;

	if(type == RewardType::correct_timely)
	{
		if(high_bw)
		{
			reward = knob::scooby_reward_hbw_correct_timely;
		}
		else
		{
			if(is_first_action)
			{
				reward = knob::scooby_reward_fa_correct_timely;
			}
			else
			{
				reward = knob::scooby_reward_correct_timely;
			}
		}
	}
	else if(type == RewardType::correct_untimely)
	{
		if(high_bw)
		{
			reward = knob::scooby_reward_hbw_correct_untimely;
		}
		else
		{
			if(is_first_action)
			{
				reward = knob::scooby_reward_fa_correct_untimely;
			}
			else
			{
				reward = knob::scooby_reward_correct_untimely;
			}
		}
	}
	else if(type == RewardType::incorrect_high_cache_acc)
	{
		if(high_bw)
		{
			reward = knob::scooby_reward_hbw_incorrect_high_cache_acc;
		}
		else
		{
			if(is_first_action)
			{
				reward = knob::scooby_reward_fa_incorrect_high_cache_acc;
			}
			else
			{
				reward = knob::scooby_reward_incorrect_high_cache_acc;
			}
		}
	}
	else if(type == RewardType::incorrect_low_cache_acc)
	{
		if(high_bw)
		{
			reward = knob::scooby_reward_hbw_incorrect_low_cache_acc;
		}
		else
		{
			if(is_first_action)
			{
				reward = knob::scooby_reward_fa_incorrect_low_cache_acc;
			}
			else
			{
				reward = knob::scooby_reward_incorrect_low_cache_acc;
			}
		}
	}
	else if(type == RewardType::none)
	{
		if(high_bw)
		{
			reward = knob::scooby_reward_hbw_none;
		}
		else
		{
			if(is_first_action)
			{
				reward = knob::scooby_reward_fa_none;
			}
			else
			{
				reward = knob::scooby_reward_none;
			}
		}
	}
	else if(type == RewardType::out_of_bounds)
	{
		if(high_bw)
		{
			reward = knob::scooby_reward_hbw_out_of_bounds;
		}
		else
		{
			if(is_first_action)
			{
				reward = knob::scooby_reward_fa_out_of_bounds;
			}
			else
			{
				reward = knob::scooby_reward_out_of_bounds;
			}
		}
	}
	else if(type == RewardType::tracker_hit)
	{
		if(high_bw)
		{
			reward = knob::scooby_reward_hbw_tracker_hit;
		}
		else
		{
			if(is_first_action)
			{
				reward = knob::scooby_reward_fa_tracker_hit;
			}
			else
			{
				reward = knob::scooby_reward_tracker_hit;
			}
		}
	}
	else
	{
		cout << "Invalid reward type found " << type << endl;
		assert(false);
	}

	return reward;
}

#endif /* SCOOBY_REWARD_H */

