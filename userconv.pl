#!/usr/bin/perl -w

# $Id: userconv.pl,v 1.1.1.1 1998/10/31 23:21:25 kluzz Exp $

# This scripts converts an old (pre 3.08) JanBot ascii userfile
# to the new format used in JanBot 3.08. It is important to run
# this converter before running 3.08 for the first time.

# Thanks to Zeev Suraski (Bourbon) for writing the skeleton for
# this script. Afterall, I am perl clueless (I have later on become
# a perl newbie, thus the script has changed a lot).


# USAGE: userconv.pl < oldfile > newfile

%conv = (
	NICK	=>	'NI',
	PASSWD	=>	'PW',
	LEVEL	=>	'LV',
	LIMIT	=>	'LM',
	DIR		=>	'DI',
	FLAGS	=>	'FL',
	RATIO	=>	'RA',
	UPLOAD	=>	'UL',
	DOWNLOAD=>	'DL',
	CREDITS	=>	'CR',
	HOST	=>	'HO',
	ALIAS 	=>	'AL'
);

for (<STDIN>)
{
	my ($del,$value)=split / +/;
	print "$conv{$del}$value";
}
