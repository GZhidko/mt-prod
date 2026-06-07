#!/usr/bin/perl -w
use CGI qw(:standard Vars);
use CGI::Carp qw(warningsToBrowser fatalsToBrowser);
use strict;
use SweetAuth;

my $host="auth.mydomain.com";

my $user=param("user");
my $password=param("password");
my $url=param("url");
my $freebeer=param("freebeer");
my $logout=param("logout");

my $octets;
my $duration;
my $idle;

my $remote_addr;

if($ENV{REMOTE_ADDR} !~ /^([\d]+)\.([\d]+)\.([\d]+)\.([\d]+)$/) {
  die "IP address error\n";
} else {
  $remote_addr = "$1.$2.$3.$4";
}

local %ENV;

print header;

if ($logout) {
  my ($rc,$ip,$a1)=sweetauth('DN', $remote_addr, '', '', 'web-logout');
  if ($rc ne "OK" || $ip ne $remote_addr) {
    print start_html("Sweetspot logout failed");
    print "UAM error $rc,$ip,$a1";
    print end_html;
    exit;
  }
  print start_html("Sweetspot logout succeeded!");
  print "<p align=CENTER><b>";
  print "Logout succeeded. <a href=\"http://$host/cgi-bin/sweetspot.cgi\">Log in again?</a>";
  print "</b></p>";
  print end_html;
  exit;
}

if ($freebeer) {
  $user="anonymous";
  $octets=30000000;
  $duration=3600;
  $idle=600;
} else {
  my $rc;
  my $str;

  #
  # User credential checking code should be here 
  # Check $user, $password
  #

  $rc = -1;
  $str = "Not implemented";

  if ($rc <= 0) {
    print start_html("Sweetspot login failed");
    if ($url) {
        print "<META HTTP-EQUIV=\"Refresh\" CONTENT=\"3;URL=$url\">";
    }
    print "Auth failed: <b>$str ($rc)</b><p>";
    if ($url) {
        print "Click <A HREF=$url> here</A> to try again.";
    }
    print end_html;
    exit;
  }
  $octets=1000000000;
  $idle=90;
  if ($rc > 86400) {
    $duration=86400;
  } else {
    $duration=$rc;
  }
}

my ($rc,$ip,$a1)=sweetauth('UP',$remote_addr,'',$octets,$octets,'','',$duration,$idle,$user,'web-login');
if ($rc ne "OK" || $ip ne $remote_addr) {
  print start_html("Sweetspot login failed");
  print "UAM error $rc,$ip,$a1";
  print end_html;
  exit;
}

print start_html("Sweetspot login succeeded");
print "<p align=CENTER><b>";
print "Login succeeded! To manage your session go to <A HREF=\"http://$host/cgi-bin/sweetspot.cgi\">http://$host/cgi-bin/sweetspot.cgi</A>";
print "</b></p>";
print "<form action=\"$url\">";
print "<table cellspacing=5 cellpadding=0 align=CENTER>";
print "<tr><td>Downstream traffic limit:<td><b>",int($octets/100)/10,"</b> KB";
print "<tr><td>Upstream traffic limit:<td><b>",int($octets/100)/10,"</b> KB";
print "<tr><td>Maximum session duration:<td><b>",int($duration/6)/10,"</b> mins";
print "<tr><td>Idle timeout:<td><b>$idle</b> seconds";
print "<tr><td align=right><input type=\"submit\" value=\"Continue\">";
print "</table>";
print "</form>";

print end_html;
