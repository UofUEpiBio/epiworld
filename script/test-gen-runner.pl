#!/usr/bin/env perl
use strict;
use warnings;
use Digest::SHA qw(sha256_hex);
use File::Path qw(make_path);
use File::Spec;

@ARGV >= 2 or die "usage: $0 NAME BUILD_DIR\n";

my ($name, $build_dir, @cov_dirs) = @ARGV;

$build_dir = File::Spec->rel2abs($build_dir);
my $test_dir = File::Spec->catdir($build_dir, ".test");
my $cov_dir  = File::Spec->catdir($build_dir, ".coverage");
my $binary   = File::Spec->catfile($build_dir, $name);
@cov_dirs = map { File::Spec->rel2abs($_) } @cov_dirs;

my $fragment = <<'EOF';
# Test case: %HUMAN_NAME%
ifeq ($(WITH_COVERAGE),1)
%COV_DIR%/coverage-%RULE_NAME%.info: COV_SILO = %COV_DIR%/coverage-%RULE_NAME%
%COV_DIR%/coverage-%RULE_NAME%.info: %TEST_DIR%/report-%RULE_NAME%.xml
	$(SAY) 'LCOV' '$(COV_SILO)/coverage.info'
	$(V)for f in %BUILD_DIR%/*.gcno; do \
		ln -sf "$$(realpath $$f)" "$(COV_SILO)/$$(basename $$f)"; \
	done
	$(V)$(LCOV) --capture --directory "$(COV_SILO)" --output-file "$(COV_SILO)/coverage.info" --quiet \
		--ignore-errors inconsistent,inconsistent,unsupported,unsupported,format,format,empty,empty,count,count,unused,unused,version,version,gcov,gcov
	$(V)$(LCOV) --extract "$(COV_SILO)/coverage.info" %COV_DIRS% --output-file "%COV_DIR%/coverage-%RULE_NAME%.info" --quiet \
		--ignore-errors inconsistent,inconsistent,unsupported,unsupported,format,format,empty,empty,count,count,unused,unused,version,version,gcov,gcov
endif

%TEST_DIR%/report-%RULE_NAME%.xml: TTYP = %TEST_DIR%/.tty-%RULE_NAME%
%TEST_DIR%/report-%RULE_NAME%.xml: COV_SILO = %COV_DIR%/coverage-%RULE_NAME%
%TEST_DIR%/report-%RULE_NAME%.xml: %BINARY%
	$(SAY) 'TEST' '%HUMAN_NAME%'
	$(V)mkdir -p $(COV_SILO)
	$(V)GCOV_PREFIX_STRIP=999 GCOV_PREFIX='$(COV_SILO)' %BINARY% \
		--reporter junit \
		--out %TEST_DIR%/report-%RULE_NAME%.xml \
		'%HUMAN_NAME%' \
		>$(TTYP) 2>&1; \
	perl -pi -e 's/name="tests"/name="%HUMAN_NAME%"/g' %TEST_DIR%/report-%RULE_NAME%.xml; \
	cat '$(TTYP)'; \
	rm '$(TTYP)'

.PHONY: %RULE_NAME%
ifeq ($(WITH_COVERAGE),1)
%RULE_NAME%: %COV_DIR%/coverage-%RULE_NAME%.info
else
%RULE_NAME%: %TEST_DIR%/report-%RULE_NAME%.xml
endif

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

my $build_abs = $build_dir;
my $test_abs  = $test_dir;
my $cov_abs   = $cov_dir;

my @targets;

sub apply_template {
    my ($template, %vars) = @_;

    for my $key (keys %vars) {
        my $value = $vars{$key};
        my $pattern = quotemeta "%$key%";
        $template =~ s/$pattern/$value/g;
    }

    return $template;
}

# TODO: Hash colisions are probably unlikely, but possible if we grow large.
for my $test (@tests) {
    my $rule = sha256_hex($test);
    push @targets, $rule;

    my $block = apply_template(
        $fragment,
        RULE_NAME => $rule,
        HUMAN_NAME => $test,
        BINARY => $binary,
        BUILD_DIR => $build_abs,
        TEST_DIR => $test_abs,
        COV_DIR => $cov_abs,
        COV_DIRS => join ' ', map { "'$_'" } @cov_dirs,
    );

    print $block;
}

print "all: @targets\n";
