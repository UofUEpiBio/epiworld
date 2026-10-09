#!/usr/bin/env perl
use strict;
use warnings;
use Digest::SHA qw(sha256_hex);
use File::Path qw(make_path);
use File::Spec;

@ARGV >= 2 or die "usage: $0 NAME BUILD_DIR\n";

my ($name, $build_dir) = @ARGV;

$build_dir = File::Spec->rel2abs($build_dir);
my $test_dir = File::Spec->catdir($build_dir, ".test");
my $binary   = File::Spec->catfile($build_dir, $name);

my $fragment = <<'EOF';
# Test case: %HUMAN_NAME%
%TEST_DIR%/report-%RULE_NAME%.xml: TTYP = %TEST_DIR%/.tty-%RULE_NAME%
%TEST_DIR%/report-%RULE_NAME%.xml: %BINARY%
	$(SAY) 'TEST' '%SHELL_NAME%'
	$(V)%BINARY% \
		--reporter junit \
		--out %TEST_DIR%/report-%RULE_NAME%.xml \
		'%SHELL_NAME%' \
		>$(TTYP) 2>&1; \
	perl -pi -e 's/name="tests"/name="%SHELL_NAME%"/g' %TEST_DIR%/report-%RULE_NAME%.xml; \
	cat '$(TTYP)'; \
	rm '$(TTYP)'

.PHONY: %RULE_NAME%
%RULE_NAME%: %TEST_DIR%/report-%RULE_NAME%.xml

EOF

print "# Auto-generated test Makefile for $name\n\n";
print ".SUFFIXES:\n";
print ".DEFAULT_GOAL := all\n\n";

my @tests;
if (open my $cmd, "-|", "$binary -l") {
    while (<$cmd>) {
        if (/^ {2}(\S.*)$/) {
            my $t = $1;
            $t =~ s/^\s+|\s+$//g;
            push @tests, $t if length $t;
        }
    }
    close $cmd;
}

my @targets;

# Test names are interpolated into single-quoted shell strings in the rules
# above, so a name containing an apostrophe (say, "the case's group") would
# close the quote early and break the generated Makefile with a shell syntax
# error rather than anything pointing at the test. Escape it the usual way.
sub shell_escape {
    my ($s) = @_;
    $s =~ s{'}{'\\''}g;
    return $s;
}

sub apply_template {
    my ($template, %vars) = @_;

    for my $key (keys %vars) {
        my $value = $vars{$key};
        my $pattern = quotemeta "%$key%";
        $template =~ s/$pattern/$value/g;
    }

    return $template;
}

# TODO: Hash collisions are probably unlikely, but possible if we grow large.
for my $test (@tests) {
    my $rule = sha256_hex($test);
    push @targets, $rule;

    my $block = apply_template(
        $fragment,
        RULE_NAME => $rule,
        HUMAN_NAME => $test,
        SHELL_NAME => shell_escape($test),
        BINARY => $binary,
        TEST_DIR => $test_dir,
    );

    print $block;
}

print "all: @targets\n";
