#!/usr/bin/env perl

use strict;
use warnings;

my $cores;

# Linux
if (-f "/proc/cpuinfo") {
    my @lines = grep { /^processor\s+:/ } do {
        open my $fh, "<", "/proc/cpuinfo" or die "cannot read cpuinfo: $!";
        <$fh>;
    };
    $cores = scalar @lines;
}

# macOS
elsif (`uname` =~ /Darwin/) {
    chomp($cores = `sysctl -n hw.ncpu`);
}

# BSD
elsif (`uname` =~ /BSD/) {
    chomp($cores = `sysctl -n hw.ncpu`);
}

# Otherwise POSIX
if (!$cores) {
    eval {
        require POSIX;
        my $n = POSIX::sysconf(&POSIX::_SC_NPROCESSORS_ONLN);
        $cores = $n if defined $n;
    };
}

# Fallback
$cores ||= 1;

print "$cores\n";