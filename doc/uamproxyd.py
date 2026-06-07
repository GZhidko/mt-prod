#!/usr/bin/python
"""ABS to UAM proxy server"""

import sys
import getopt
import traceback
from time import time
try:
    from absware import adapter
except ImportError:
    sys.stderr.write('The absware Python package is missing!\n')
    sys.exit(-1)
try:    
    import uamclient
except ImportError:
    sys.stderr.write('The uamclient Python package is missing!\n')
    sys.exit(-1)
from uamproxy.error import UamError
from uamproxy import log, daemon

def uamError(**req):
    return {'rc': -100501, 'str': req.get('str', 'general error')}

#def getInnerIp(**req):
#    if 'snat_ip' in req and 'snat_port' in req:
#        rsp = uamclient.Status(**uamclient.Inner(**req).sync()).sync()
#        if 'ip' in rsp:
#            req['ip'] = rsp['ip']
#    if 'ip' not in req:
#        raise UamError('Missing client address info')
#    return req

def getInnerIp(**req):
    try:
        rsp = uamclient.Status(**uamclient.Inner(**req).sync(timeout=your_timeout)).sync()
        if 'ip' in rsp:
            req['ip'] = rsp['ip']
    except UamError as e:
        sys.stderr.write(f"Error getting inner IP: {e}\n")
        # Handle the error or raise it if necessary
        raise e

    if 'ip' not in req:
        raise UamError('Missing client address info')

    return req


def uamStatus(**req):
    return uamclient.Status(**getInnerIp(**req)).sync(purge=True)

def uamUsage(**req):
    return uamclient.Limits(
        **uamclient.Counters(**getInnerIp(**req)).sync(purge=True)
    ).sync()

def uamUpdate(**req):
    status = uamStatus(**req)
    if 'ip' not in req:
        req['ip'] = status['ip']
    if 'state' not in req:
        req['state'] = status['state']
    if 'state_id' not in req:
        req['state_id'] = status['state_id']
    if 'context' in req:
        status['context'].update(**req['context'])
        req['context'] = status['context']
    if req['state'] == 'UP':
        return uamclient.Status(
            **uamclient.Open(**req).sync(purge=True)
        ).sync()
    elif req['state'] == 'DN':
        return uamclient.Status(
            **uamclient.Close(**req).sync(purge=True)
        ).sync()
    else:
        raise UamError('Unknown UAM state requested')

def uamHandoverStart(**req):
    status = uamStatus(**req)
    if status['state'] == 'UP':
        context = status['context']
        if 'context' in req and \
                context.get('state_id', '?') != req['context'].get('state_id', '?'):
            status['rc'], status['str'] = 5, 'Mismatched context ID'
            return status
        counters = uamclient.Counters(**status).sync()
        limits = uamclient.Limits(**status).sync()
        context['duration'] = max(
            0, limits['max_duration'] - counters['duration'] - counters['idle']
        )
        context['idle'] = limits['max_idle']
        # so far just a single counter is available in context
        context['octets_in'] = max(
            max(0, long(limits['max_octets_in']) - \
                long(counters['octets_in'])
            ),
            max(0, long(limits['max_octets_out']) - \
                long(counters['octets_out'])
            )
        )
        context['octets_out'] = context['octets_in']
        uamclient.Close(event_context='Roamed-Out', **status).sync()
        return uamclient.Open(
            ip=status['ip'],
            filter_name=status['filter_name'],
            context=context,
            event_context='Handover-In-Progress',
            max_duration=req.get('max_duration', 30),
            max_idle=req.get('max_duration', 30),
            max_octets_in=req.get('max_octets_in', 5000000),
            max_octets_out=req.get('max_octets_out', 5000000)
        ).sync()
    else:
        status['str'] = 'Session already down'
        return status

def uamHandoverFinish(**req):
    status = uamStatus(**req)
    if status['state'] == 'UP':
        uamclient.Close(event_context='Handover-Requested', **status).sync()
    if 'context' in req:
        status['context'].update(**req['context'])
    return uamclient.Open(
        ip=status['ip'],
        filter_name=req.get(
            'filter_name', status['filter_name']
        ),
        interim_interval=req.get(
            'interim_interval', status['interim_interval']
        ),
        max_octets_in=req.get(
            'max_octets_in', status['context']['octets_in']
        ),
        max_octets_out=req.get(
            'max_octets_out', status['context']['octets_in'] # octets_in!
        ),
        max_duration=req.get(
            'max_duration', status['context']['duration']
        ),
        max_idle=req.get(
            'max_idle', status['context']['idle']
        ),
        context=status['context'],
        event_context=status['state'] == 'UP' and 'Handover-Succeeded' or 'Roamed-In'
    ).sync()

def uamRetention(**req):
    return uamclient.Retention(**req).sync(purge=True)
    
opMap = {
    'status': uamStatus,
    'usage': uamUsage,
    'update': uamUpdate,
    'hstart': uamHandoverStart,
    'hfinish': uamHandoverFinish,
    'retention': uamRetention,
    'error': uamError
}

def flattenAbsTable(dct):
    r = {}
    for k in dct:
        if dct[k]:
            r[k] = dct[k][0]
        else:
            r[k] = None
        if isinstance(r[k], dict):
            r[k] = flattenAbsTable(r[k])
    return r

def columnizeUamMsg(dct):
    r = { 'rc': [ 0 ], 'str': [ 'How you doin?' ] }
    for k in dct:
        if k in ('octets_in', 'octets_out',
                 'max_octets_in', 'max_octets_out',
                 'max_bps_in', 'max_bps_out'):
            dct[k] = str(dct[k])
        if isinstance(dct[k], dict):
            r[k] = columnizeUamMsg(dct[k])
            del r[k]['rc']; del r[k]['str']
            r[k] = [ r[k] ]
        else:
            r[k] = [ dct[k] ]
    return r

def handleAbsMsg(absAdapter, serial, req, stat={}):
    if stat:
        if stat['_total'] == 1000:
            stat['elapsed'] = '%.2f' % (time()-stat['_created'])
            stat['throughput'] = '%.2f' % (stat['_total']/(time()-stat['_created']))
            log.msg('stat: %s' % ', '.join(['%s=%s' % x for x in stat.items() if x[0][0] != '_']))
            stat.update(_total=1, success=0, failure=0,
                        _created=time(), total=stat['total']+1000)
        else:    
            stat['_total'] += 1
    else:
        stat.update(_total=1, success=0, failure=0, _created=time(), total=0)
    stat['failure'] += 1

    if debug:
        log.msg('ABS request #%s, %r' % (serial, req))
    req = flattenAbsTable(req)
    op = req.get('op', 'error')
    if not op:
        op = 'error'
    handler = opMap.get(op, uamError)
    try:
        rsp = handler(**req)
    except:
        exc_info = sys.exc_info()
        log.msg('error for request #%s, request %r: %r' % (serial, req, exc_info[1]))
        for line in traceback.format_exception(*exc_info):
            log.msg(line.replace('\n', ';'))
        rsp = uamError(str=str(exc_info[1]))
    if debug:
        log.msg('ABS response #%s, %r' % (serial, rsp))
    absAdapter.sendRsp(columnizeUamMsg(rsp), serial)
    stat['success'] += 1; stat['failure'] -= 1
    
# global state variables

debug = False
absServiceId = 100500
absQueueDepth = 5
foregroundFlag = False
loggingMethod = []
procUser = procGroup = None
pidFile = '/var/run/abs/uamproxy.pid'
uamConfigFile = '/usr/local/etc/sweetspot/sweetuam.conf'
        
helpMessage = """\
Usage: %s [--help]
    [--version ]
    [--debug]
    [--foreground]
    [--process-user=<uname>] [--process-group=<gname>]
    [--pid-file=<file>]
    [--logging-method=<%s[:args>]>]
    [--uam-config-file=<file>]
    [--uam-response-timeout=<float>]
    [--abs-service-id=<int>]
    [--abs-queue-depth=<int>]""" % (
    sys.argv[0],
    '|'.join(log.gMap.keys())
)

try:
    opts, params = getopt.getopt(sys.argv[1:], 'hv', [
        'help', 'version', 'debug', 'foreground',
        'process-user=', 'process-group=',
        'pid-file=', 'logging-method=',
        'uam-config-file=', 'uam-response-timeout=',
        'abs-service-id=', 'abs-queue-depth='
    ])
except Exception:
    sys.stderr.write('ERROR: %s\r\n%s\r\n' % (sys.exc_info()[1], helpMessage))
    sys.exit(-1)

if params:
    sys.stderr.write('ERROR: extra arguments supplied %s\r\n%s\r\n' % (params, helpMessage))
    sys.exit(-1)

log.setLogger('uamproxyd', 'stderr')

for opt in opts:
    if opt[0] == '-h' or opt[0] == '--help':
        sys.stderr.write("""\
Synopsis:
  ABS service provisioning daemon talking to Sweetspot Network Access
  Controller through UAM.
Documentation:
  http://wiki.abs.gldn.net/
%s
""" % helpMessage)
        sys.exit(-1)
    if opt[0] == '-v' or opt[0] == '--version':
        import absware
        import uamclient
        import uamproxy
        sys.stderr.write("""\
UAMProxy version %s, written by Ilya Etingof <elie@rol.ru>
Using foundation libraries: absware %s, uamclient %s.
Python interpreter: %s
Software documentation at http://sweetspot.sourceforge.net
%s
""" % (uamproxy.__version__, absware.__version__, uamclient.__version__, sys.version, helpMessage))
        sys.exit(-1)
    elif opt[0] == '--debug':
        debug = True
    elif opt[0] == '--foreground':
        foregroundFlag = True
    elif opt[0] == '--process-user':
        procUser = opt[1]
    elif opt[0] == '--process-group':
        procGroup = opt[1]
    elif opt[0] == '--pid-file':
        pidFile = opt[1]
    elif opt[0] == '--logging-method':
        try:
            loggingMethod = opt[1].split(':')
        except UamError:
            sys.stderr.write('%s\r\n%s\r\n' % (sys.exc_info()[1], helpMessage))
            sys.exit(-1)
    elif opt[0] == '--uam-config-file':
        uamConfigFile = opt[1]
    elif opt[0] == '--uam-response-timeout':
        try:
            uamclient.uamTimeout = float(opt[1])
        except:
            sys.stderr.write('Bad UAM timeout: %s\r\n%s\r\n' % (sys.exc_info()[1], helpMessage))
            sys.exit(-1)            
    elif opt[0] == '--abs-service-id':
        try:
            absServiceId = int(opt[1])
        except ValueError:
            sys.stderr.write('Bad ABS service ID: %s\r\n%s\r\n' % (sys.exc_info()[1], helpMessage))
            sys.exit(-1)
    elif opt[0] == '--abs-queue-depth':
        try:
            absQueueDepth = int(opt[1])
        except ValueError:
            sys.stderr.write('Bad ABS service queue depth: %s\r\n%s\r\n' % (sys.exc_info()[1], helpMessage))
            sys.exit(-1)

if procUser and procGroup:
    try:
        daemon.dropPrivileges(procUser, procGroup)
    except:
        sys.stderr.write('ERROR: cant drop priveleges: %s\r\n%s\r\n' % (sys.exc_info()[1], helpMessage))
        sys.exit(-1)

if not foregroundFlag:
    try:
        daemon.daemonize(pidFile)
    except:
        sys.stderr.write('ERROR: cant daemonize process: %s\r\n%s\r\n' % (sys.exc_info()[1], helpMessage))
        sys.exit(-1)
    else:
        if not loggingMethod:    
            log.setLogger('uamproxy', 'syslog', 'local7', 'info')

if loggingMethod:
    log.setLogger('uamproxy', *loggingMethod)

try:        
    log.msg('Initializing UAM client with config %s, response timeout %s secs...' % (uamConfigFile, uamclient.uamTimeout))
    uamclient.setupUamClient(uamConfigFile)

    log.msg('Starting ABS provider %d, queue depth %d...' % (absServiceId, absQueueDepth))
    adapter.absServer((absServiceId, absQueueDepth, handleAbsMsg))
except KeyboardInterrupt:
    log.msg('Shutting down process...')
except Exception:
    log.msg('Process terminated')
    for line in traceback.format_exception(*sys.exc_info()):
        log.msg(line.replace('\n', ';'))

