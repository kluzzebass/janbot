#!/usr/bin/perl -w

# $Id: sanity.pl,v 1.1.1.1 1998/10/31 23:21:25 kluzz Exp $

if (!$ARGV[0]) {
  print "No file specified.\n";
  exit;
}

open FI,"<$ARGV[0]" or die "Unable to open $ARGV[0]: $!\n";

$lnum=1;
$fatals=0;
$warnings=0;
$verbose=0;

while (<FI>)
{
	chomp;

	$key=substr($_,0,2);
	$data=substr($_,2);

	if ( substr($_,0,4) eq "NICK" )
	{
		print "Error! This is an old userlist! Use the userconv.pl script to convert\nto the new format.\n";
		exit(1);
	}
	elsif ( $key eq "NI" ) { do_nick($data); }
	elsif ( $key eq "PW" ) { do_password($data); }
	elsif ( $key eq "LV" ) { do_level($data); }
	elsif ( $key eq "LM" ) { do_limit($data); }
	elsif ( $key eq "DI" ) { do_dir($data); }
	elsif ( $key eq "FL" ) { do_flags($data); }
	elsif ( $key eq "RA" ) { do_ratio($data); }
	elsif ( $key eq "UL" ) { do_upload($data); }
	elsif ( $key eq "DL" ) { do_download($data); }
	elsif ( $key eq "CR" ) { do_credits($data); }
	elsif ( $key eq "HO" ) { do_host($data); }
	elsif ( $key eq "AL" ) { do_alias($data); }

	else { key_error($key,$data); }

	$lnum++;
}

printf("---\nFound $fatals fatal error(s) and $warnings warning(s).\n");

sub key_error
{
	printf("  [%5d] Line contains illegal key: %s\n",$lnum,@_);
	$fatals++;
}

sub do_nick
{
	my($nick)=@_;
	if ($verbose) { printf("Checking record: %s\n",$nick); }
	my $len=length($nick);
	if (($len<1)||($len>9))
	{
		printf("  [%5d] Odd nick length: %d (should be 1 to 9 characters)\n",$lnum,$len);
		$warnings++;
		return;
	}
	my $stripped=$nick;
	if ($stripped=~tr/a-zA-Z0-9{}|[]\-_^//!=$len)
	{
		printf("  [%5d] Illegal characters in nick: %s\n",$lnum,$stripped);
		$fatals++;
	}
}

sub do_password
{
	my($pwd)=@_;
	my $len=length($pwd);
	if ($len!=13)
	{
		printf("  [%5d] Odd password length: %d (encrypted passwords are 13 characters)\n",$lnum,$len);
		$fatals++;
		return;
	}
	my $stripped=$pwd;
	if ($stripped=~tr/a-zA-Z0-9.\///!=$len)
	{
		printf("  [%5d] Illegal characters in password: %s\n",$lnum,$stripped);
		$fatals++;
	}
}

sub do_level
{
	my($lvl)=@_;
	if (($lvl>3)||($lvl<0))
	{
		printf("  [%5d] Illegal user level: %s\n",$lnum,$lvl);
		$fatals++;
	}
}

sub do_limit
{
	my($lim)=@_;
	if ($lim<0)
	{
		printf("  [%5d] Illegal DCC limit: %s\n",$lnum,$lim);
		$fatals++;
	}
	elsif ($lim>50)
	{
		printf("  [%5d] Odd DCC limit: %s\n",$lnum,$lim);
		$warnings++;
	}
}

sub do_dir
{
	my($dir)=@_;
	my $len=length($dir);
	if ($len==0)
	{
		printf("  [%5d] Empty directory string.\n",$lnum);
		$warnings++;
	}
	elsif(substr($dir,0,1) ne "/")
	{
		printf("  [%5d] Directory string doesn't start with '/'.\n",$lnum);
		$warnings++;
	}
}

sub do_flags
{
	my($flags)=@_;
	if ($flags & 0xffffffe0)
	{
		printf("  [%5d] Unknown flag settings: %d.\n",$lnum,$flags);
		$warnings++;
	}
}

sub do_ratio
{
	my($ratio)=@_;
	if ($ratio<0)
	{
		printf("  [%5d] Illegal ratio value: %d\n.",$lnum,$ratio);
		$fatals++;
	}
	elsif($ratio>50)
	{
		printf("  [%5d] Odd ratio value: %d.\n",$lnum,$ratio);
		$warnings++;
	}
}

sub do_upload
{
	my($upload)=@_;
	if ($upload<0)
	{
		printf("  [%5d] Impossible upload value: %d\n",$lnum,$upload);
		$fatals++;
	}
}

sub do_download
{
	my($download)=@_;
	if ($download<0)
	{
		printf("  [%5d] Impossible download value: %d\n",$lnum,$download);
		$fatals++;
	}
}

sub do_credits
{
	my($credits)=@_;
	if ($credits<0)
	{
		printf("  [%5d] Impossible credits value: %d\n",$lnum,$credits);
		$fatals++;
	}
}

sub do_host
{
	my($hostmask)=@_;
	my ($user,$host)=split(/@/,$hostmask);
	my $len=length($host);
	unless (defined($host) && defined($user) && $host && $user)
	{
		printf("  [%5d] Illegal hostmask: %s\n",$lnum,$hostmask);
	}
	elsif ($host=~tr/a-zA-Z0-9.?*-//!=$len)
	{
		printf("  [%5d] Illegal characters in hostmask: %s\n",$lnum,$host);
		$fatals++;
	}
	$user=""; # Shut the fuck up, perl
}

sub do_alias
{
#	printf("do_alias() has been called\n");
}


