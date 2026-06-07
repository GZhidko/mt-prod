#!/bin/sh
#
# Downstream routing setup script
#
# Usage:  downstream.sh <cidr> <start|stop>
#
# This script will be called for each either user-networks or 
# snat-public-networks CIDR configured to sweetspot.
#
case "$2" in
'start')
  /sbin/ip rule add to $1 lookup 111
  ;;
'stop')
  /sbin/ip rule del to $1 lookup 111
  ;;
esac

