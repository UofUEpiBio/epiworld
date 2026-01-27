#!/usr/bin/env perl
use strict;
use warnings;
use XML::LibXML;

my $file = shift or die "usage: $0 report.xml\n";
my $doc = XML::LibXML->load_xml(location => $file);
my $fail_count = 0;

for my $suite ($doc->findnodes('/testsuites/testsuite')) {
    my $f = $suite->getAttribute('failures') // 0;
    my $e = $suite->getAttribute('errors')   // 0;
    $fail_count += $f + $e;
}

exit $fail_count;
