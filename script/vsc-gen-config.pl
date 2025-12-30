#!/usr/bin/env perl

use strict;
use warnings;
use JSON::PP;

my $json = JSON::PP->new->utf8->pretty->canonical;

my $do_help = grep { $_ eq '-h' } @ARGV;
my ($program_database, $test_database) = @ARGV;

if ($do_help || !$program_database || !$test_database) {
    print <<"EOF";
Usage: $0 <program_database> <test_database>
Generate a set of Visual Studio Code configuration files.
Options:
  -h        Show this help message
EOF
    exit 0;
}

# The program database is in the format:
#
#   D <program path>: <source file 1> <source file 2> ...
#
# Similarly, the test database is in the format:
#
#   T <test file>: <test name 1> <test name 2> ...
# 
# We will parse these files to generate the necessary configuration files.
# For example, we will create a tasks.json file to compile the programs
# and a launch.json file to run the tests.
#
# Each file will get an entry that allows it to be built, and then the whole
# program linked (e.g. calling out to Make for the program that source file
# belongs to), and then another to do the same but run the test binary with
# the test names specified in the test basebase as arguments.

my %programs;
open my $prog_db_fh, '<', $program_database or die "Could not open '$program_database': $!";
while (my $line = <$prog_db_fh>) {
    chomp $line;
    if ($line =~ /^D\s+(\S+):\s+(.*)$/) {
        my $program_path = $1;
        my @source_files = split /\s+/, $2;
        $programs{$program_path} = \@source_files;
    }
}
close $prog_db_fh;

my %tests;
open my $test_db_fh, '<', $test_database or die "Could not open '$test_database': $!";
while (my $line = <$test_db_fh>) {
    chomp $line;
    if ($line =~ /^T\s+(\S+):\s+(.*)$/) {
        my $test_file = $1;
        my $rest = $2;
        my @test_names = ($rest =~ /"([^"]+)"/g);

        $tests{$test_file} = \@test_names;
    }
}
close $test_db_fh;

# Print for debugging

use Data::Dumper;
print "Programs:\n", Dumper(\%programs);
print "Tests:\n", Dumper(\%tests);

# Generate tasks.json.
my @tasks;
for my $program_path (keys %programs) {
    my $program_name = (split '/', $program_path)[-1];
    push @tasks, {
        label => "Build $program_name",
        type => "shell",
        command => "make ",
        "args" => [
            '-j$(perl script/sys-get-nproc.pl)',
            $program_path
        ],
        group => "build",
        problemMatcher => ["\$gcc"],
    };
}

# Generate launch.json.
my @configurations;
for my $test_file (keys %tests) {
    my $test_name = (split '/', $test_file)[-1];
    push @configurations, {
        name => "Run tests in $test_name",
        type => "cppdbg",
        request => "launch",
        program => "\${workspaceFolder}/$test_file",
        args => $tests{$test_file},
        preLaunchTask => "Build $test_name",
        stopAtEntry => \0,
        cwd => "\${workspaceFolder}",
        environment => [],
        externalConsole => \1,
        MIMode => "gdb",
        setupCommands => [
            {
                description => "Enable pretty-printing for gdb",
                text => "-enable-pretty-printing",
                ignoreFailures => \1,
            },
        ],
    };
}

print $json->encode({
    #tasks => \@tasks,
    configurations => \@configurations,
});