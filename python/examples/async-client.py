#!/usr/bin/env python2

import sweetuam, select

sweetuam.set_debug_flags('all')

cf = sweetuam.config_load('../../doc/sweetuam.conf')
print 'Configuration file %s loaded' % cf

uamClient = sweetuam.UamClient()

uamClient.sendMsg('UP', '192.168.102.1')
r, w, e = select.select([ uamClient.fileno() ], [], [], 1)
if r:
    print uamClient.recvMsg()
else:
    print 'select error'

del uamClient
