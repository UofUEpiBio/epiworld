#!/usr/bin/env perl
use strict;
use warnings;
use XML::LibXML;

my $out = XML::LibXML::Document->new('1.0', 'UTF-8');
my $root = $out->createElement('testsuites');
$out->setDocumentElement($root);

for my $file (@ARGV) {
    my $doc = XML::LibXML->load_xml(location => $file);
    for my $suite ($doc->findnodes('/testsuites/testsuite')) {
        # Import into the output document so the node belongs to it
        my $imported = $suite->cloneNode(1);
        $root->appendChild($imported);
    }
}

print $out->toString(1);
