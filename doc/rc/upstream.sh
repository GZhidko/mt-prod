#!/bin/sh
#
# Upstream routing setup script
#
# Usage:  upstream.sh <cidr> <start|stop>
#
# This script will be called for each user-networks CIDR configured to
# sweetspot.
#
case "$2" in
'start')
  /sbin/ip rule add from $1 lookup 110
  ;;
'stop')
  /sbin/ip rule del from $1 lookup 110
  ;;
esac
