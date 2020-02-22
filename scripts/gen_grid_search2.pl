#!/usr/bin/perl

use warnings;

my @alpha_list = (0.01, 0.02, 0.05, 0.1, 0.2, 0.3, 0.5, 0.75);
my @gamma_list = (0.5, 0.75);
my @epsilon_list = (0.01, 0.1, 0.2, 0.5);

for $alpha (@alpha_list)
{
	for $gamma (@gamma_list)
	{
		for $epsilon (@epsilon_list)
		{
			print "scooby_gs_a_${alpha}_g_${gamma}_e_${epsilon}  \$(BASE) \$(SCOOBY) --scooby_alpha=${alpha} --scooby_gamma=${gamma} --scooby_epsilon=${epsilon}\n";
		}
	}
}
