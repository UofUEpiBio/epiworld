use strict;
use warnings;
use File::Spec;
use Cwd qw(abs_path);

my $line78 = '/' x 78;

our $N_CALLS   = 0;
our $MAX_CALLS = 200;
our %SEEN_FILES;

sub unfolder {
    my ($txt_ref, $rel) = @_;
    $rel //= "include/epiworld/";

    # Find lines with includes
    my @heads = grep { $$txt_ref[$_] =~ /^\s*#include\s+\"/ } 0..$#$txt_ref;

    return $txt_ref unless @heads;

    $N_CALLS++;

    if ($N_CALLS >= $MAX_CALLS) {
        warn "Max calls reached.\n";
        return $txt_ref;
    }

    my @fns = map {
        my $line = $$txt_ref[$_];
        $line =~ s/^[^\"]+\"|\"$//gr;
    } @heads;

    my $head_start = sprintf(
        "/*%s\n%s//\n\n Start of -%%s-\n\n%s//\n%s*/\n\n",
        $line78, $line78, $line78, $line78
    );

    my $head_end = sprintf(
        "/*%s\n%s//\n\n End of -%%s-\n\n%s//\n%s*/\n\n",
        $line78, $line78, $line78, $line78
    );

    my @new_src;
    my %head_idx = map { $heads[$_] => $_ } 0..$#heads;

    for my $loc (0 .. $#$txt_ref) {
        my $line = $$txt_ref[$loc];

        if (!exists $head_idx{$loc}) {
            push @new_src, $line;
            next;
        }

        my $h = $head_idx{$loc};
        my $fn  = $rel . $fns[$h];
        $fn =~ s/^\s+|\s+$//g;
        my $canonical = abs_path($fn) // File::Spec->canonpath($fn);

        if ($SEEN_FILES{$canonical}) {
            push @new_src, "// (already included $fn)";
            next;
        }

        $SEEN_FILES{$canonical} = 1;

        open my $fh, '<', $fn or do {
            warn "Could not read $fn: $!\n";
            push @new_src, $line;
            next;
        };
        my @tmp_lines = <$fh>;
        chomp @tmp_lines;
        close $fh;

        # Extract relative path
        my $sub_rel = $fns[$h];
        $sub_rel =~ s{[^/]+$}{};
        my $new_rel = File::Spec->catdir($rel, $sub_rel) . '/';

        my $tmp_ref = unfolder(\@tmp_lines, $new_rel);

        my $start_comment = sprintf($head_start, $fn);
        my $end_comment   = sprintf($head_end,   $fn);

        push @new_src, $start_comment, @$tmp_ref, $end_comment;
    }

    return \@new_src;
}

# Main
my $rel = shift @ARGV // "include/epiworld/";
$rel .= '/' unless $rel =~ m{/$};
my $root_file = shift @ARGV // "epiworld.hpp";
my $input_file = File::Spec->catfile($rel . $root_file);

open my $in, '<', $input_file or die "Cannot read $input_file: $!";
my @src = <$in>;
chomp @src;
close $in;

my $src_new_ref = unfolder(\@src, $rel);

# Emit to stdout instead of writing a file
print join("\n", @$src_new_ref), "\n";
