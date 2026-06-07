# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl SweetAuth.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use Test;
BEGIN { plan tests => 2 };
use SweetAuth;
ok(1); # If we made it this far, we're ok.
my @res=sweetauth('ST','194.154.66.102','a','b','c','d','e');
if (@res) {
	warn "\n\t",join(',',@res),"\n";
	ok(1);
} else {
	ok(0);
}

#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.

