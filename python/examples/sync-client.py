#!/usr/bin/env python2

import sweetuam

sweetuam.set_debug_flags('all')

print 'version: ' + sweetuam.__version__

cf = sweetuam.config_load('../../doc/sweetuam.conf')
print 'Configuration file %s loaded' % cf

uamClient = sweetuam.UamClient()

print repr(uamClient('ST', '172.17.200.22'))

del uamClient

sweetuam.config_unload()
