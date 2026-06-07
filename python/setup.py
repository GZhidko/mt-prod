from distutils.core import setup, Extension
import os

version = '0.0.3'
src = '../src'

sw_version = os.environ.get('SW_VERSION')
if sw_version:
    version += '/sw' + sw_version

setup(name = 'sweetuam',
    version = version,
    author = 'Alexey Michurin',
    author_email = 'am@rol.ru',
    url = 'mailto:am@rol.ru',
    license = 'The BSD License',
    description = 'A Python module for implementing UAM clients.',
    long_description = 'The sweetuam module defines an UAM client '
                       'session object that can be used to send commands '
                       'and receive replays to/from sweetspot daemon.',
    platforms = ('Unix',),
    ext_modules = [Extension(
        name = 'sweetuam',
        sources = ['sweetuammodule.c'],
        include_dirs = (src, '..'),
        define_macros = [
            ('SW_DEFAULT_CONFIG_DIR', '"/usr/local/etc/sweetspot"'),
            ('SW_MODULE_VERSION', '"%s"' % version)
        ],
        library_dirs = (src,),
#       runtime_library_dirs = (src,),
        language = 'c',
        extra_link_args = ['%s/%s.o' % (src, x) for x in (
            'debug', 'tlog', 'cfg', 'uamclt', 'uammsg', 'pretty'
        )]
    )]
)
