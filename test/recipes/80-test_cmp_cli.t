#! /usr/bin/env perl
# Copyright OpenSSL 2007-2018
# Copyright Nokia 2007-2018
# Copyright Siemens AG 2015-2018
#
# Contents licensed under the terms of the OpenSSL license
# See https://www.openssl.org/source/license.html for details
#
# SPDX-License-Identifier: OpenSSL
#
# CMP tests by Martin Peylo, Tobias Pankert, and David von Oheimb.

use strict;
use warnings;

use POSIX;
use OpenSSL::Test qw/:DEFAULT with srctop_dir data_file/;
use OpenSSL::Test::Utils;
use Data::Dumper; # for debugging purposes only

my $proxy = '""';
$proxy = $ENV{http_proxy} if $ENV{http_proxy};
$proxy =~ s{http://}{};

setup("test_cmp_cli");

plan skip_all => "CMP is not supported by this OpenSSL build"
    if disabled("cmp");

my @cmp_basic_tests = (
    [ "output help",                      [ "-help"], 0 ],
    [ "unknown CLI parameter",            [ "-asdffdsa"], 1 ],
    [ "bad int syntax: non-digit",        [ "-msgtimeout", "a/" ], 1 ],
    [ "bad int syntax: float",            [ "-msgtimeout", "3.14" ], 1 ],
    [ "bad int syntax: trailing garbage", [ "-msgtimeout", "314_+" ], 1 ],
    [ "bad int: out of range",            [ "-msgtimeout", "2147483648" ], 1 ],
);

my $test_config = "test_config.cnf";

# the CA server configuration consists of:
                # The CA name (implies directoy with certs etc. and CA-specific section in config file)
my $ca_cn;      # The CA common name
my $secret;     # The secret for PBM
my $column;     # The column number of the expected result
my $sleep = 0;  # The time to sleep between two requests

sub load_config {
    my $name = shift;
    open (CH, $test_config) or die "Can't open $test_config: $!";
    $ca_cn = undef;
    $secret = undef;
    $column = undef;
    $sleep = undef;
    my $active = 0;
    while (<CH>) {
        if (m/\[\s*$name\s*\]/) {
            $active = 1;
            } elsif (m/\[\s*.*?\s*\]/) {
                $active = 0;
        } elsif ($active) {
            $ca_cn = $1 if m/\s*recipient\s*=\s*(.*)?\s*$/;
            $secret = $1 if m/\s*pbm_secret\s*=\s*(.*)?\s*$/;
            $column = $1 if m/\s*column\s*=\s*(.*)?\s*$/;
                $sleep = $1 if m/\s*sleep\s*=\s*(.*)?\s*$/;
        }
    }
    close CH;
    die "Can't find all CA config values in $test_config section [$name]\n"
        if !defined $ca_cn || !defined $secret || !defined $column || !defined $sleep;
}

my @ca_configurations = ("EJBCA", "Insta");
@ca_configurations = ( $ENV{CMP_CONF} ) if $ENV{CMP_CONF};

my @all_aspects = ("connection", "verification", "credentials", "commands");

sub test_cmp_cli {
    my @args = @_;
    my $name = shift;
    my $title = shift;
    my $params = shift;
    my $expected_exit = shift;
    with({ exit_checker => sub {
        my $OK = shift == $expected_exit;
        print Dumper @args if !($ENV{HARNESS_VERBOSE} == 2 && $OK); # for debugging purposes only
        return $OK; } },
         sub { ok(run(app(["openssl", "cmp", @$params,])),
                  $title); });
}

sub test_cmp_cli_aspect {
    my $name = shift;
    my $aspect = shift;
    my $tests = shift;
    subtest "CMP app CLI $name $aspect\n" => sub {
        plan tests => scalar @$tests;
        foreach (@$tests) {
          SKIP: {
              test_cmp_cli($name, $$_[0], $$_[1], $$_[2]);
              sleep($sleep);
            }
        }
    };
}

indir data_file() => sub { # TODO: replace by data_dir() when available
    plan tests => 1 + @ca_configurations * @all_aspects;

    test_cmp_cli_aspect("basic", "", \@cmp_basic_tests);

    # TODO: complete and thoroughly review _all_ of the around 500 test cases
    foreach my $name (@ca_configurations) {
        load_config($name);
        indir $name => sub {
            foreach my $aspect (@all_aspects) {
                my $tests = load_tests($name, $aspect);
                test_cmp_cli_aspect($name, $aspect, $tests);
            };
        };
    };
};

sub load_tests {
        my $name = shift;
        my $aspect = shift;
	my $file = data_file("test_$aspect.csv");
	my @result;

	open(my $data, '<', $file) || die "Cannot load $file\n";
	LOOP: while (my $line = <$data>) {
		chomp $line;
		next LOOP if $line =~ m/TLS/i; # skip tests requiring TLS
		$line =~ s{\r\n}{\n}g; # adjust line endings
		$line =~ s{_ISSUINGCACN}{$ca_cn}g;
		$line =~ s{_SECRET}{$secret}g;
		$line =~ s{-section;;}{-proxy;$proxy;-config;../$test_config;-section;$name,$aspect;};
		my @fields = grep /\S/, split ";", $line;
		my $expected_exit = $fields[$column];
		my $title = $fields[2];
		next LOOP if (!defined($expected_exit) or ($expected_exit ne 0 and $expected_exit ne 1));
		@fields = grep {$_ ne 'BLANK'} @fields[3..@fields-1];
		push @result, [$title, \@fields, $expected_exit];
	}
	return \@result;
}
