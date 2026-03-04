#!/usr/bin/env perl

use strict;
use warnings;

my $do_help = grep { $_ eq '-h' } @ARGV;
my $emit_tag = grep { $_ eq '-t' } @ARGV;

if ($do_help) {
    print <<"EOF";
Usage: $0 [-t] < test-file > test-names
Extract test case names/tags from a source file containing TEST_CASE definitions.
Options:
  -t        Emit test tags instead of test names
  -h        Show this help message
EOF
    exit 0;
}

local $/;                       # read entire input as one string
my $text = <STDIN>;

while ($text =~ /(?:EPIWORLD_)?TEST_CASE\s*\(\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\)/sg) {
    if ($emit_tag) {
        print "$2\n";
    } else {
        print "$1\n";
    }
}
