#!/usr/bin/perl

use warnings;

$incorr_high_acc_min 		= -10;
$incorr_high_acc_max 		= 5;
$incorr_low_acc_min 		= -20;
$incorr_low_acc_max 		= 0;
$pref_acc_thresh_min		= 10;
$pref_acc_thresh_max		= 75;

my $total_exps = 0;
for(my $incorr_high_acc = $incorr_high_acc_min; $incorr_high_acc <= $incorr_high_acc_max; $incorr_high_acc += 1)
{
	for(my $incorr_low_acc = $incorr_low_acc_min; $incorr_low_acc <= $incorr_low_acc_max; $incorr_low_acc += 1)
	{
		for(my $pref_acc_thresh = $pref_acc_thresh_min; $pref_acc_thresh <= $pref_acc_thresh_max; $pref_acc_thresh += 5)
		{
			print STDOUT "scooby_gs3_incorr_high_${incorr_high_acc}_low_${incorr_low_acc}_acc_thresh_${pref_acc_thresh}   \$(BASE) \$(SCOOBY) --scooby_reward_incorrect_high_cache_acc=${incorr_high_acc} --scooby_reward_incorrect_low_cache_acc=${incorr_low_acc} --scooby_pref_acc_thresh=${pref_acc_thresh}\n";
			$total_exps++;
		}
	}
}

print STDERR "Total experiemtns: $total_exps";

