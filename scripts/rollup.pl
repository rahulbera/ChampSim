#!/usr/bin/perl

use lib '/mnt/panzer/rahbera/ChampSim/scripts';
use warnings;
use Getopt::Long;
use Trace;
use Exp;
use Metric;
use Statistics::Descriptive;
use Data::Dumper;

# defaults
my $CHAMPSIM_ROOT="/mnt/panzer/rahbera/ChampSim";
my $tlist_file = "tlist";
my $exp_file = "exp";
my $mfile = "mfile";
my $ext = "out";

GetOptions(	'tlist=s' => \$tlist_file,
			'exp=s' => \$exp_file,
			'mfile=s' => \$mfile,
			'ext=s' => \$ext,
			) or die "Usage: $0 --exe <executable> --exp <exp file> --tlist <trace list>\n";

my @trace_info = Trace::parse($tlist_file);
my @exp_info = Exp::parse($exp_file);
my @m_info = Metric::parse($mfile);
my $stat = Statistics::Descriptive::Full->new();
$Statistics::Descriptive::Tolerance = 1e-12;

print "Trace,Exp";
for $metric (@m_info)
{
	my $mname = $metric->{"NAME"};
	print ",".$metric->{"NAME"};
}
print ",Filter\n";
# print Dumper(\@m_info);
# print Dumper(\@trace_info);

for $trace (@trace_info)
{
	my $trace_name = $trace->{"NAME"};
	my $passed = 1;
	my %records;
	for $exp (@exp_info)
	{
		my $exp_name = $exp->{"NAME"};
		my $log_file = "${trace_name}_${exp_name}.${ext}";
		my @metric_values;
		
		if (-e $log_file)
		{
			open(my $fh, '<', $log_file) or die "cannot open $log_file\n";
			while($line = <$fh>)
			{
				chomp($line);
				if($ext eq "stats")
				{
					my ($key, $value) = split /=/, $line;
					$key = trim($key);
					$value = trim($value);
					$records{$key} = $value;
				}
				else
				{
					my $space_count = () = $line =~ / /g;
					if ($space_count == 1)
					{
						my ($key, $value) = split / /, $line;
						$key = trim($key);
						$value = trim($value);
						$records{$key} = $value;						
					}
				}
			}
			close($fh);

			for $metric (@m_info)
			{
				$metric_name = $metric->{"NAME"};
				$metric_type = $metric->{"TYPE"};
				# print "metric: $metric_name type: $metric_type\n";
				if(exists($records{$metric_name}))
				{
					$value = $records{$metric_name};
					# print "value: $value\n";
					my @tokens = split(',', $value);
					$stat->clear();
					$stat->add_data(@tokens);
					if($metric_type eq "sum")
					{
						$value = $stat->sum();
					}
					elsif($metric_type eq "mean")
					{
						$value = $stat->mean();
					}
					elsif($metric_type eq "nzmean")
					{
						$stat->clear();
						@tokens = grep {trim($_)} @tokens;
					       	$stat->add_data(@tokens);
						$value = $stat->mean();
					}
					elsif($metric_type eq "min")
					{
						$value = $stat->min();
					}
					elsif($metric_type eq "max")
					{
						$value = $stat->max();
					}
					elsif($metric_type eq "standard_deviation")
					{
						$value = $stat->standard_deviation();
					}
					elsif($metric_type eq "variance")
					{
						$value = $stat->variance();
					}
					else
					{
						die "invalid summary type\n";
					}
				}
				else
				{
					$value = 0;
					$passed = 0;
				}
				push(@metric_values, $value);
			}
		}
		else
		{
			$passed = 0;
			for $metric (@m_info)
			{
				push(@metric_values, 0);
			}
		}
		my $arr = join(",", @metric_values);
		# print "$trace_name,$exp_name,";
		# print "$arr,FAILED_STATUS\n";
		my $result = sprintf("%s,%s,%s,FAILED_STATUS\n", $trace_name, $exp_name, $arr);
		$result =~ s/FAILED_STATUS/$passed/g;
		print $result;
	}
}

sub  trim { my $s = shift; $s =~ s/^\s+|\s+$//g; return $s };
