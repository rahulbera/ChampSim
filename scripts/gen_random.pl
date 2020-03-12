#!/usr/bin/perl

use lib '/mnt/panzer/rahbera/ChampSim/scripts';
use strict;
use warnings;
use Crypt::Random qw( makerandom ); 

my $bitsize = 32;
my $rand = makerandom ( Size => $bitsize, Strength => 1, Uniform => 1 );
say $rand;
