#!/usr/bin/env perl
#
# Generate a compile_commands.json for C/C++ header files in a specified directory.
# Arguments to this script should likely be passed from inside Make.
#
# Usage:
#   gencdb.pl --dir include --cppflags="-Iinclude -DMYDEF=1" --cxxflags="-std=c++20"
#

use strict;
use warnings;
use File::Find;
use Cwd 'abs_path';
use JSON;

my %args = (
    dir       => './include',
    cppflags  => '',
    cxxflags  => '',
    compiler  => 'clang++',
);

# Arg parsing
for (my $i = 0; $i < @ARGV; $i++) {
    if ($ARGV[$i] eq '--dir') {
        $args{dir} = $ARGV[++$i];
    } elsif ($ARGV[$i] eq '--cppflags') {
        $args{cppflags} = $ARGV[++$i];
    } elsif ($ARGV[$i] eq '--cxxflags') {
        $args{cxxflags} = $ARGV[++$i];
    } elsif ($ARGV[$i] eq '--compiler') {
        $args{compiler} = $ARGV[++$i];
    } else {
        die "Unknown argument: $ARGV[$i]\n";
    }
}

my $root = abs_path($args{dir});
my @headers;

# Collect headers recursively.
find(
    sub {
        return unless -f $_;
        return unless /\.(h|hpp|hh|hxx)$/;
        push @headers, $File::Find::name;
    },
    $root
);

my @entries;
foreach my $file (@headers) {
    my $entry = {
        directory => $root,
        file      => $file,
        command   => join(' ', $args{compiler}, $args{cxxflags}, $args{cppflags}, '-c', '-x', 'c++', $file, '-o', '/dev/null'),
    };
    push @entries, $entry;
}

# Out
my $json = JSON->new->utf8->pretty->encode(\@entries);
print $json;
