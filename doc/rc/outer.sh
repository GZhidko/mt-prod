#!/bin/sh
#
# TUN interface startup script
#
# Usage:  outer.sh <inner-dev-name> <outer-dev-name>
#
# This script will be called upon downstream TUN pair creation.
#
/sbin/ifconfig $1 192.168.0.5 pointopoint 192.168.0.6 up
/sbin/ifconfig $2 192.168.0.7 pointopoint 192.168.0.8 up
# will source-route downstream traffic through outer p-to-p interface
/sbin/ip route add default via 192.168.0.8 table 111

