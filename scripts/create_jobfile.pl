#!/usr/bin/perl

use lib '/mnt/panzer/rahbera/ChampSim/scripts';
use warnings;
use Getopt::Long;
use Trace;
use Exp;

# defaults
my $CHAMPSIM_ROOT="/mnt/panzer/rahbera/ChampSim";
my $tlist_file = "tlist";
my $exp_file = "exp";
my $exe;
my $local = "0";
my $ncores = 1;
my $slurm_partition = "slurm_part";
my $exclude_list;

GetOptions('tlist=s' => \$tlist_file,
	   'exp=s' => \$exp_file,
	   'exe=s' => \$exe,
	   'ncores=s' => \$ncores,
	   'local=s' => \$local,
	   'exclude=s' => \$exclude_list,
) or die "Usage: $0 --exe <executable> --exp <exp file> --tlist <trace list>\n";
die "Supply exe\n" unless defined $exe;

my $exclude_nodes_list = "kratos[$exclude_list]" if defined $exclude_list;

my @trace_info = Trace::parse($tlist_file);
my @exp_info = Exp::parse($exp_file);
if($ncores == 0)
{
	print "have to supply -ncores\n";
	exit 1;
}

# preamble for sbatch script
if($local eq "0")
{
	print "#!/bin/bash -l\n";
	print "#\n";
	print "#\n";
}

print "#\n";
print "# Traces:\n";
foreach $trace (@trace_info)
{
	my $trace_name = $trace->{"NAME"};
	print "#    $trace_name\n";
}
print "#\n";

print "#\n";
print "# Experiments:\n";
foreach $exp (@exp_info)
{
	my $exp_name = $exp->{"NAME"};
	my $exp_knobs = $exp->{"KNOBS"};
	print "#    $exp_name: $exp_knobs\n";
}
print "#\n";
print "#\n";
print "#\n";
print "#\n";

foreach $trace (@trace_info)
{
	foreach $exp (@exp_info)
	{
		my $exp_name = $exp->{"NAME"};
		my $exp_knobs = $exp->{"KNOBS"};
		my $trace_name = $trace->{"NAME"};
		my $trace_input = $trace->{"TRACE"};
		my $trace_knobs = $trace->{"KNOBS"};

		my $cmdline;
		if($local)
		{
			$cmdline = "$exe $exp_knobs $trace_knobs -traces $trace_input";
		}
		else
		{
			$slurm_cmd = "sbatch -p $slurm_partition --mincpus=1 --exclude=${exclude_nodes_list} -c $ncores -J ${trace_name}_${exp_name} -o ${trace_name}_${exp_name}.out -e ${trace_name}_${exp_name}.err";
			$cmdline = "$slurm_cmd $CHAMPSIM_ROOT/wrapper.sh $exe \"$exp_knobs $trace_knobs -traces $trace_input\"";
		}
		$cmdline =~ s/\$(EXP)/$exp_name/g;
		$cmdline =~ s/\$(TRACE)/$trace_name/g;
		$cmdline =~ s/\$(NCORES)/$ncores/g;
		print "$cmdline\n";
	}
}
