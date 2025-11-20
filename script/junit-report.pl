#!/usr/bin/env perl
use strict;
use warnings;
use XML::LibXML;
use Term::ANSIColor qw(color);

my ($short, $haiku) = (0, 0);

while (@ARGV && $ARGV[0] =~ /^--/) {
    my $flag = shift @ARGV;
    if    ($flag eq '--short') { $short = 1 }
    elsif ($flag eq '--haiku') { $haiku = 1 }
    else  { die "unknown flag: $flag\n" }
}

my $file = shift or die "usage: $0 [--short] [--haiku] report.xml\n";
my $doc    = XML::LibXML->load_xml(location => $file);
my @suites = $doc->findnodes('/testsuites/testsuite');
my ($total_tests, $total_failures, $total_errors) = (0, 0, 0);

for my $suite (@suites) {
    my %attr = map {
        $_ => ($suite->getAttribute($_) // 0)
    } qw(name tests failures errors time);

    $total_tests    += $attr{tests};
    $total_failures += $attr{failures};
    $total_errors   += $attr{errors};

    if ($haiku) {
        my $col = ($attr{failures} || $attr{errors}) ? 'red' : 'green';
        print color($col), ".", color('reset');
        next;
    }

    if ($short) {
        if ($attr{failures} || $attr{errors}) {
            print color('red'),
                  "  Failures: $attr{failures}  Errors: $attr{errors}\t",
                  color('reset');
        } else {
            print color('green'), "  All tests passed\t", color('reset');
        }
        
        print color('bold'), "$attr{name}: $attr{tests} tests", color('reset'), "\n";
        next;
    }

    print color('bold'), "Suite: $attr{name}\n", color('reset');
    print "  Tests: $attr{tests}\n";
    print "  Time:  $attr{time}\n";

    if ($attr{failures} || $attr{errors}) {
        print color('red'),
              "  Failures: $attr{failures}  Errors: $attr{errors}\n",
              color('reset');
    } else {
        print color('green'), "  All tests passed\n", color('reset');
    }

    for my $case ($suite->findnodes('testcase')) {
        my $cname = $case->getAttribute('name')      // '';
        my $class = $case->getAttribute('classname') // '';

        my $status;
        if    ($case->findnodes('failure')) { $status = ['FAIL',  'red'] }
        elsif ($case->findnodes('error'))   { $status = ['ERROR', 'red'] }
        else                                { $status = ['OK',    'green'] }

        print color($status->[1]),
              "    $status->[0]: $class :: $cname\n",
              color('reset');
    }

    print "\n";
}

unless ($haiku || $short) {
    print "\n", color('bold'), "Overall summary\n", color('reset');
    print "  Total tests: $total_tests\n";
    if ($total_failures || $total_errors) {
        print color('red'),
              "  Failures: $total_failures  Errors: $total_errors\n",
              color('reset');
    } else {
        print color('green'), "  All tests passed\n", color('reset');
    }
}

print "\n" if $haiku;
