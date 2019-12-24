#!/usr/bin/perl

use warnings;

for(my $rew_corr_timely = 8; $rew_corr_timely <= 15; $rew_corr_timely += 2)
{
	for(my $rew_corr_untimely = 0; $rew_corr_untimely <= 7; $rew_corr_untimely += 2)
	{
		for(my $rew_incorr = -7; $rew_incorr <= 2; $rew_incorr += 2)
		{
			for(my $rew_none = -9; $rew_none <= 0; $rew_none += 2)
			{
				for(my $rew_out_of_bounds = -9; $rew_out_of_bounds <= 0; $rew_out_of_bounds += 2)
				{
					print "scooby_gs_${rew_corr_timely}_${rew_corr_untimely}_${rew_incorr}_${rew_none}_${rew_out_of_bounds}   \$(BASE) \$(SCOOBY) --scooby_reward_correct_timely=${rew_corr_timely} --scooby_reward_correct_untimely=${rew_corr_untimely} --scooby_reward_incorrect=${rew_incorr} --scooby_reward_none=${rew_none} --scooby_reward_out_of_bounds=${rew_out_of_bounds}\n";
				}
			}
		}
	}
} 

