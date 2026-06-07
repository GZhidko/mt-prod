package SweetAuth;

use 5.000;
use strict;
use Carp;

require Exporter;
require DynaLoader;
use AutoLoader;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $AUTOLOAD);
@ISA = qw(Exporter
	DynaLoader);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use SweetAuth ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
%EXPORT_TAGS = ( 'all' => [ qw(
	
) ] );

@EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

@EXPORT = qw(
	sweetauth
);

$VERSION = '0.01';

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.

    my $constname;
    ($constname = $AUTOLOAD) =~ s/.*:://;
    croak "&SweetAuth::constant not defined" if $constname eq 'constant';
    my ($error, $val) = constant($constname);
    if ($error) { croak $error; }
    {
	no strict 'refs';
	# Fixed between 5.005_53 and 5.005_61
#XXX	if ($] >= 5.00561) {
#XXX	    *$AUTOLOAD = sub () { $val };
#XXX	}
#XXX	else {
	    *$AUTOLOAD = sub { $val };
#XXX	}
    }
    goto &$AUTOLOAD;
}

bootstrap SweetAuth $VERSION;

# Preloaded methods go here.

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__
# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

SweetAuth - synchronous Perl interface for SweetAuth control API

=head1 SYNOPSIS

  use SweetAuth;
  my @result=sweetauth($event,$ip,$arg1,$arg2,$arg3,$arg4,$arg5,$arg6,$arg7,$arg8,$arg9);
  if (@result) { # call successfull }
  else { # some failure, error messages may be on stderr }

=head1 DESCRIPTION

Send SweetSpot's remote control message and wait for result.
On entry, first two arguments are mandatory, other depend on
the type of operation.  All arguments are strings.  $event may
be further classified as follows:

session management: 'ST', 'UP' or 'DN', for 'status', 'up' or 'down'.
session accounting: 'LI', 'CU' and 'PE' for counters retrieval 
(limits, current, peak accordingly).
SNAT resolution: 'OU' and 'IN' for live public->private and
private->public endpoints resolution.

Other arguments are dependant on the $event. See `sweetuam --help`
for detailed description.

=head1 EXPORT

  sweetauth

=head1 SEE ALSO

http://sweetspot.sourceforge.net/

=head1 AUTHOR

Eugene Crosser, E<lt>crosser@average.orgE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2006 by Eugene Crosser

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.8.7 or,
at your option, any later version of Perl 5 you may have available.


=cut
