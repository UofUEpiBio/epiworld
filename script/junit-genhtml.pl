#!/usr/bin/env perl
use strict;
use warnings;
use XML::LibXML;
use POSIX qw(strftime);

my $file = shift or die "usage: $0 report.xml > report.html\n";

my $doc    = XML::LibXML->load_xml(location => $file);
my @suites = $doc->findnodes('/testsuites/testsuite');

my $generated = strftime("%Y-%m-%d %H:%M:%S", localtime);

print <<"HTML";
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>Test Results</title>
<style>
body { font-family: sans-serif; margin: 30px; max-width: 900px; }
h1 { margin-bottom: 0.5em; }
h2 { margin-top: 1.2em; }
.suite { margin-bottom: 3em; }
.pass { color: #0a0; font-weight: bold; }
.fail { color: #c00; font-weight: bold; }
.metrics { display: inline-block; margin-bottom: 1.2em; font-size: 0.95em; width: 300px; }
.metrics div { display: flex; justify-content: space-between; border-bottom: 1px solid #ddd; padding: 4px 0; max-width: 280px; }
.metrics .label { color: #555; font-weight: 600; }
table { border-collapse: collapse; margin-top: 0.5em; width: 100%; table-layout: fixed; }
th { background: #f0f0f0; text-align: left; }
td, th { border: 1px solid #ccc; padding: 6px 10px; word-wrap: break-word; }
footer { margin-top: 3em; font-size: 0.8em; color: #666; }
</style>
</head>
<body>

<h1>Test Results</h1>
HTML

my ($total_tests, $total_failures, $total_errors) = (0, 0, 0);

for my $suite (@suites) {
    my $name     = $suite->getAttribute('name')     // '';
    my $tests    = $suite->getAttribute('tests')    // 0;
    my $failures = $suite->getAttribute('failures') // 0;
    my $errors   = $suite->getAttribute('errors')   // 0;

    $total_tests    += $tests;
    $total_failures += $failures;
    $total_errors   += $errors;

    print qq{<div class="suite">\n};
    print qq{<h2>$name</h2>\n};
    print qq{
    <div class="metrics">
      <div><span class="label">Tests</span><span>$tests</span></div>
      <div><span class="label">Failures</span><span>$failures</span></div>
      <div><span class="label">Errors</span><span>$errors</span></div>
    </div>
    };

    print qq{<table>\n<tr><th>Name</th><th>Status</th></tr>\n};

    for my $case ($suite->findnodes('testcase')) {
        my $cname = $case->getAttribute('name')      // '';

        my ($status, $css) = ("OK", "pass");
        if ($case->findnodes('failure')) {
            ($status, $css) = ("FAIL", "fail");
        } elsif ($case->findnodes('error')) {
            ($status, $css) = ("ERROR", "fail");
        }

        print qq{<tr><td>$cname</td><td class="$css">$status</td></tr>\n};
    }

    print qq{</table>\n};
    print qq{</div>\n};
    print qq{<hr>\n};
}

print <<"HTML";
<div class="metrics">
  <h2>Overall Summary</h2>
  <div><span class="label">Total tests</span><span>$total_tests</span></div>
  <div><span class="label">Failures</span><span>$total_failures</span></div>
  <div><span class="label">Errors</span><span>$total_errors</span></div>
</div>

<footer>
Generated on: $generated
</footer>

</body>
</html>
HTML
