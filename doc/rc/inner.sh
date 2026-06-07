#!/bin/sh
#
# TUN interface startup script
#
# Usage:  inner.sh <inner-dev-name> <outer-dev-name>
#
# This script will be called upon upstream TUN pair creation.
#
/sbin/ifconfig $1 192.168.0.1 pointopoint 192.168.0.2 up
/sbin/ifconfig $2 192.168.0.3 pointopoint 192.168.0.4 up
# will source-route upstream traffic through inner p-to-p interface
/sbin/ip route add default via 192.168.0.2 table 110
