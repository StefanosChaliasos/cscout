#!/usr/bin/perl
#
# Spy on gcc invocations and construct corresponding CScout directives
#
# (C) Copyright 2005 Diomidis Spinellis
#
# This file is part of CScout.
#
# CScout is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# CScout is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with CScout.  If not, see <http://www.gnu.org/licenses/>.
# Parsing the switches appears to be a tar pit (see incomplete version 1.1).
# Documentation is incomplete and inconsistent with the source code.
# Different gcc versions and installations add and remove options.
# Therefore it is easier to let gcc do the work
#

use Cwd 'abs_path';

$debug = 0;

# Gather input / output files and remove them from the command line
for ($i = 0; $i <= $#ARGV; $i++) {
	$arg = $ARGV[$i];
	if ($arg =~ m/\.c$/i) {
		push(@cfiles, $arg);
	} elsif ($arg =~ m/^-o(.+)$/ || $arg =~ m/^--output=(.*)/) {
		$output = $1;
		next;
	} elsif ($arg eq '-o' || $arg eq '--output') {
		$output = $ARGV[++$i];
		next;
	} elsif ($arg =~ m/^-L(.+)$/ || $arg =~ m/^--library-directory=(.*)/) {
		push(@ldirs, $1);
		next;
	} elsif ($arg eq '-L' || $arg eq '--library-directory') {
		push(@ldirs, $ARGV[++$i]);
		next;
	} elsif ($arg =~ m/^-l(.+)$/ || $arg =~ m/^--library=(.*)/) {
		push(@libs, $1);
		next;
	} elsif ($arg eq '-l' || $arg eq '--library') {
		push(@libs, $ARGV[++$i]);
		next;
	} elsif ($arg =~ m/\.(o|obj)$/i) {
		push(@ofiles, $arg);
		next;
	} elsif ($arg =~ m/\.a$/i) {
		push(@afiles, $arg);
		next;
	} else {
		push(@ARGV2, $arg);
	}
	$bailout = 1 if (
		($arg =~ m/^-M/ || $arg =~ m/-dependencies/) &&
		$arg !~ '-MD' && $arg !~ '-MMD' &&
		$arg !~ "--write-dependencies" &&
		$arg !~ "--write-user-dependencies");
	$bailout = 1 if ($arg eq '--preprocess' || $arg eq '-E');
	$bailout = 1 if ($arg eq '--assemble' || $arg eq '-S');
	$bailout = 1 if ($arg =~ m/-print-file-name/);
	$compile = 1 if ($arg eq '--compile' || $arg eq '-c');
}

# We don't handle assembly files or preprocessing
if ($bailout) {
	print STDERR "Just run ($ENV{CSCOUT_SPY_REAL_GCC} @ARGV))\n" if ($debug);
	$exit = system(($ENV{CSCOUT_SPY_REAL_GCC}, @ARGV)) / 256;
	print STDERR "Just run done ($exit)\n" if ($debug);
	exit $exit;
}

if ($#cfiles >= 0) {
	push(@ARGV2, $ENV{CSCOUT_SPY_TMPDIR} . '/empty.c');
	$cmdline = $ENV{CSCOUT_SPY_REAL_GCC} . ' ' . join(' ', @ARGV2);
	print STDERR "Running $cmdline\n" if ($debug);

	# Gather include path
	open(IN, "$cmdline -v -E 2>&1|") || die "Unable to run $cmdline: $!\n";
	undef $gather;
	while (<IN>) {
		chop;
		$gather = 1 if (/\#include \"\.\.\.\" search starts here\:/);
		next if (/ search starts here\:/);
		last if (/End of search list\./);
		if ($gather) {
			s/^\s*//;
			push(@incs, '#pragma includepath "' . abs_path($_) . '"');
			print STDERR "path=[$_]\n" if ($debug > 2);
		}
	}

	# Gather macro definitions
	open(IN, "$cmdline -dD -E|") || die "Unable to run $cmdline: $!\n";
	while (<IN>) {
		chop;
		next if (/\s*\#\s*\d/);
		push(@defs, $_);
	}
}

open(RULES, $rulesfile = ">>$ENV{CSCOUT_SPY_TMPDIR}/rules") || die "Unable to open $rulesfile: $!\n";

$origline = "gcc " . join(' ', @ARGV);
$origline =~ s/\n/ /g;

# Output compilation rules
for $cfile (@cfiles) {
	print RULES "BEGIN COMPILE\n";
	print RULES "CMDLINE $origline\n";
	print RULES "INSRC " . abs_path($cfile) . "\n";
	if ($compile && $output) {
		$coutput = $output;
	} else {
		$coutput= $cfile;
		$coutput =~ s/\.c$/.o/i;
		$coutput =~ s,.*/,,;
	}
	print RULES "OUTOBJ " . abs_path($coutput) . "\n";
	print RULES join("\n", @incs), "\n";
	print RULES join("\n", @defs), "\n";
	print RULES "END COMPILE\n";
}

if (!$compile && $#cfiles >= 0 || $#ofiles >= 0) {
	print RULES "BEGIN LINK\n";
	print RULES "CMDLINE $origline\n";
	if ($output) {
		print RULES "OUTEXE $output\n";
	} else {
		print RULES "OUTEXE a.out\n";
	}
	for $cfile (@cfiles) {
		$output= $cfile;
		$output =~ s/\.c$/.o/i;
		print RULES "INOBJ " . abs_path($output) . "\n";
	}
	for $ofile (@ofiles) {
		print RULES "INOBJ " . abs_path($ofile) . "\n";
	}
	for $afile (@afiles) {
		print RULES "INLIB " . abs_path($afile) . "\n";
	}
	for $libfile (@libfiles) {
		for $ldir (@ldirs) {
			if (-r ($try = "$ldir/lib$libfile.a")) {
				print RULES "INLIB " . abs_path($try) . "\n";
				last;
			}
		}
	}
	print RULES "END LINK\n";
}

# Finally, execute the real gcc
print STDERR "Finally run ($ENV{CSCOUT_SPY_REAL_GCC} @ARGV))\n" if ($debug);
exit system(($ENV{CSCOUT_SPY_REAL_GCC}, @ARGV)) / 256;
