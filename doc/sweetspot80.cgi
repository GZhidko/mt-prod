#!/usr/bin/perl -wT
#
# An alternative to this URI splitting script could be the following
# Apache virtual host setup:
#
# ...
#
# NameVirtualHost auth.mydomain.com:80
#
# <VirtualHost auth.mydomain.com:80>
#         ServerName auth.mydomain.com
#         ServerAdmin support@auth.mydomain.com
# </VirtualHost>

# <VirtualHost _default_:80>
#         ServerName redirect
#         ServerAdmin support@redirect
#         RewriteEngine   on
#         RewriteRule   ^(.*)$  http://auth.mydomain.com/cgi-bin/sweetspot.cgi?url=http://%{HTTP_HOST}%{REQUEST_URI}%{QUERY_STRING} [R=302,L]
# </VirtualHost>
#
use CGI qw(:standard Vars);
use CGI::Carp qw(warningsToBrowser fatalsToBrowser);
use strict;

my $host="auth.mydomain.com";

my $http_host=$ENV{HTTP_HOST};
my $uri=$ENV{REQUEST_URI};
my $sweeturl="http://$host/cgi-bin/sweetspot.cgi";

print header;
print start_html("Sweetspot welcomes you!");

print "<HTML><HEAD><TITLE>Re-directing to sweetspot</TITLE>";
print "<META HTTP-EQUIV=\"Refresh\" CONTENT=\"0;URL=$sweeturl?url=$http_host$uri\">";
print "</HEAD><BODY bgcolor=\"white\">";
print "Redirecting <A HREF=$sweeturl?url=$http_host$uri>here</A>.";
print "</BODY></HTML>";

print end_h;
