#!/usr/bin/perl -wT
use CGI qw(:standard Vars);
use CGI::Carp qw(warningsToBrowser fatalsToBrowser);
use strict;
use SweetAuth;

my $host="auth.mydomain.com";

my $url=param("url");
my $user=param("user");
my $remote_addr; 

if($ENV{REMOTE_ADDR} !~ /^([\d]+)\.([\d]+)\.([\d]+)\.([\d]+)$/) {
  die "IP address error\n";
} else {
  $remote_addr = "$1.$2.$3.$4";
}

local %ENV;

print header;

my ($rc,$ip,$a1,$a2,$a3,$a4)=sweetauth('ST', $remote_addr);
if ($rc ne "OK") {
  print start_html("Sweetspot went faulty");
  print "UAM error $rc,$ip,$a1";
  print end_h;
  exit;
}
if ($a1 eq "UP") {  
  print start_html("Sweetspot session management");
  print "<p align=CENTER><b>";
  print "To terminate your sweet session, click the Logout button below";
  print "</b></p>";
  print "<form action=\"http://$host/cgi-bin/sweetauth.cgi\" method=\"POST\">";
  print "<table cellspacing=5 cellpadding=0 align=CENTER>";

  my ($rc,$ip,$a1,$a2,$a3,$a4,$a5,$a6)=sweetauth('CN', $remote_addr);
  if ($rc eq "OK") {
    print "<tr><td>Downlink volume:<td><b>",int($a1/100)/10,"</b> KB";
    print "<tr><td>Uplink volume:<td><b>",int($a2/100)/10,"</b> KB";
    print "<tr><td>Max downlink rate:<td><b>$a3</b> bps";
    print "<tr><td>Max uplink rate:<td><b>$a4</b> bps";
o    print "<tr><td>Duration:<td><b>",int($a5/6)/10,"</b> mins";
  }
  print "<tr><td align=right><input type=\"submit\" name=logout value=\"Logout\">";
  print "</table>";
  print "</form>";
  print end_h;
} else {
  print start_html("Sweetspot welcomes you!");
  print "<p align=CENTER><b>";
  print "You have been captured! Authenticate with us to get out of here.";
  print "</b></p>";
  print "<form action=\"https://$host/cgi-bin/sweetauth.cgi\" method=\"POST\">";
  print "<table cellspacing=5 cellpadding=0 align=CENTER>";
  print "<tr><td>Username:<td><input type=\"text\" name=\"user\" value=\"$user\">";
  print "<tr><td>Password:<td><input type=\"password\" name=\"password\">";
  print "<tr><td><td><input type=\"submit\" value=\"Login\">";
  print "&nbsp<input type=\"submit\" name=freebeer value=\"Free beer\">";
  print "</table>";
  print "<input type=\"hidden\" name=url value=\"$url\">";
  print "</form>";
  print end_h;
}
