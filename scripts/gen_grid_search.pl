#!/usr/bin/perl

use warnings;

$corr_timely_min 	= 8;
$corr_timely_max 	= 24;
$corr_untimely_min 	= 2;
$corr_untimely_max 	= 16;
$incorr_min 		= -12;
$incorr_max 		= 2;
$none_min 		= -16;
$none_max 		= -2;
$out_of_bounds_min 	= -12;
$out_of_bounds_max 	= -2;

my $total_exps = 0;
for(my $rew_corr_timely = $corr_timely_min; $rew_corr_timely <= $corr_timely_max; $rew_corr_timely += 2)
{
	for(my $rew_corr_untimely = $corr_untimely_min; $rew_corr_untimely <= $corr_untimely_max; $rew_corr_untimely += 2)
	{
		for(my $rew_incorr = $incorr_min; $rew_incorr <= $incorr_max; $rew_incorr += 2)
		{
			for(my $rew_none = $none_min; $rew_none <= $none_max; $rew_none += 3)
			{
				for(my $rew_out_of_bounds = $out_of_bounds_min; $rew_out_of_bounds <= $out_of_bounds_max; $rew_out_of_bounds += 3)
				{
					print STDOUT "scooby_gs15May_${rew_corr_timely}_${rew_corr_untimely}_${rew_incorr}_${rew_none}_${rew_out_of_bounds}   \$(BASE) \$(SCOOBY_CMAC) --scooby_reward_correct_timely=${rew_corr_timely} --scooby_reward_correct_untimely=${rew_corr_untimely} --scooby_reward_incorrect=${rew_incorr} --scooby_reward_none=${rew_none} --scooby_reward_out_of_bounds=${rew_out_of_bounds}\n";
					$total_exps++;
				}
			}
		}
	}
}

print STDERR "Total experiemtns: $total_exps";

