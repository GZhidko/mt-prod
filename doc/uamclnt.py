import sweetuam
import select
import sys
import random
import base64
from time import time

__version__ = '0.0.3'

uamTimeout = 0.1

random.seed()

class UamError(Exception): pass

class AbstractOperation(dict):
    send = receive = ()
    required = set()
    code = '?'
    ttl = 0.0
    def __init__(self, **kwargs):
        self.required = set(self.required)
        if not self.required.issubset(kwargs):
            raise UamError('not all required keys present: %s' %
                           (self.required.difference(kwargs),))
        for k in kwargs:
            if isinstance(kwargs[k], dict):
                kwargs[k] = kwargs[k].copy()
        self._locked = True
        super(AbstractOperation, self).__init__(**kwargs)
        self._postsync()

    def __setitem__(self, k, v):
        if k in self.required and v not in (self[k], str(self[k])) and \
                self._locked:
            raise UamError('required key %s is immutable' % k)
        super(AbstractOperation, self).__setitem__(k, v)

    def update(self, E, **F):
        for D in E, F:
            for k in self.required.intersection(D):
                if D[k] not in (self.get(k, D[k]), str(self.get(k, D[k]))) \
                        and self._locked:
                    raise UamError('required key %s is immutable' % k)
        super(AbstractOperation, self).update(E, **F)

    def pop(self, k, d=None):
        if k in self.required and self._locked:
            raise UamError('required key is immutable')
        return super(AbstractOperation, self).pop(k, d)

    def popitem(self):
        if self.required.issuperset(self) and self._locked:
            raise UamError('cant pop required key(s)')
        k = self.required.symmetric_difference(self)[0]
        return k, self.pop(k)

    def _presync(self): return []
    def _postsync(self): pass
    def sync(purge=False, ttl=None, timeout=None):
        raise NotImplementedError

class AbstractReadOperation(AbstractOperation):
    ints = ()
    def sync(self, purge=False, ttl=None, timeout=None):
        if not hasattr(self, '__timestamp'):
            self.__timestamp = 0
        if self.ttl <= 0 or \
                time() - self.__timestamp > ttl is not None and \
                ttl or self.ttl:
            serial = '%s' % random.randrange(1, 0xffffffff)
            uamClient.sendMsg(
                serial,
                self.get('state-id', '0'),
                self.code,
                *[ str(self.get(k, '')) for k in self.send ]
            )
            while True:
                r, w, x = select.select(
                    [uamClient.fileno()], [], [], timeout or uamTimeout
                )
                if not r:
                    raise UamError('read operation timeout (serial %s)' % serial)
                rsp = uamClient.recvMsg()
                if serial != rsp[0]:
                    continue 
                else:
                    break
            if rsp[2] != 'OK':
                raise UamError('sweetspot error: %s' % rsp[3])
            if purge: self.clear()
            self.update(dict(zip(self.receive, rsp[3:])))
            self._postsync()
            self['state_id'] = rsp[1]
            if self.ttl > 0:
                self.__timestamp = time()
        return self

    def _postsync(self):
        for k in self.ints:
            if k in self and self[k]:
                self._locked = False
                self[k] = int(self[k])
                self._locked = True

class AbstractWriteOperation(AbstractOperation):
    def sync(self, purge=False, ttl=None, timeout=None):
        serial = '%s' % random.randrange(1, 0xffffffff)
        uamClient.sendMsg(serial, self.get('state_id', '0'), self.code,
                          *self._presync())
        while True:
            r, w, x = select.select(
                [uamClient.fileno()], [], [], timeout or uamTimeout
            )
            if not r:
                raise UamError('write operation timeout (serial %s)' % serial)
            rsp = uamClient.recvMsg()
            if serial != rsp[0]:
                continue 
            else:
                break
        if rsp[2] != 'OK':
            raise UamError('sweetspot error: %s' % rsp[3])
        if purge: self.clear()
        self.update(dict(zip(self.receive, rsp[3:])))
        self._postsync()
        self['state_id'] = rsp[1]
        return self

class Context(dict):
# future fields
#    fields = 'version', 'state_id', \
#             'dnis', 'nasip', 'channel', 'clid', 'user', 'class', 'msid', \
#             'rname', 'units', 'status', \
#             'octets_in', 'octets_out', 'duration', 'idle'
#    ints = 'version', 'channel', 'duration', 'idle'
#    bins = 'state_id', 'class', 'user', 'status'
    fields = 'dnis', 'clid', 'nasip', 'user', 'class', 'msid', 'rname', \
             'units', 'octets_in', 'duration', 'idle', 'channel', \
             'state_id', 'status'
    ints = 'duration', 'idle', 'channel'
    # XXX should just check for conflicting symbols
    bins = 'user', 'class', 'state_id', 'status'

    def serialize(self):
        d = self.copy()
        for k in d:
            if d[k]:
                try:
                    if k in self.bins:
                        d[k] = str(d[k])
                        if not d[k].isalnum():
                            d[k] = 'BASE64/'+base64.encodestring(d[k])[:-1]
                except:
                    d[k] = ''
        return ','.join([str(d.get(k, '')) for k in self.fields])

    def deserialize(self, blob):
        d = {}
        try:
            values = blob.split(',')
        except:
            values = []
        shortage = len(self.fields) - len(values)
        if shortage > 0:
            values.extend(['']*shortage)
        for k,v in zip(self.fields, values):
            if v:
                try:
                    if k in self.bins and v.find('BASE64/') == 0:
                        v = base64.decodestring(v[7:])
                    elif k in self.ints:
                        v = int(v)
                except:
                    if k in self.ints:
                        v = 0
                    else:
                        v = ''
            elif k in self.ints:
                v = 0
            d[k] = v
        return d

    def load(self, blob):
        self.update(**self.deserialize(blob))
        return self
        
class Status(AbstractReadOperation):
    code = 'ST'
    send = requred = 'ip',
    receive = 'ip', 'state', 'session_id', 'filter_name', \
              'interim_interval', 'session_context'
    ints = 'interim_interval',

    def _postsync(self):
        super(Status, self)._postsync()
        if 'context' not in self:
            self['context'] = Context()
        self['context'].update(
            **Context().load(self.get('session_context', ''))
        )

class Open(AbstractWriteOperation):
    code = 'UP'
    send = 'ip', 'filter_name', 'interim_interval', 'max_octets_in', \
           'max_octets_out', 'max_bps_in', 'max_bps_out', \
           'max_duration', 'max_idle', 'session_context', 'event_context'
    receive = required = 'ip',

    def _presync(self):
        if 'context' in self:
            if not isinstance(self['context'], Context):
                self['context'] = Context(**self['context'])
        elif 'session_context' in self:
            self['context'] = Context().load(
                self.get('session_context', '')
            )
        else:
            self['context'] = Context()

        return [ k == 'session_context' and \
                 self['context'] and \
                 self['context'].serialize() or \
                 str(self.get(k, '')) for k in self.send ]

class Close(AbstractWriteOperation):
    code = 'DN'
    send = 'ip', 'filter_name', 'session_context', 'event_context'
    receive = required = 'ip',

    def _presync(self):
        if 'context' in self:
            if not isinstance(self['context'], Context):
                self['context'] = Context(**self['context'])
        elif 'session_context' in self:
            self['context'] = Context().load(
                self.get('session_context', '')
            )
        else:
            self['context'] = Context()

        return [ k == 'session_context' and \
                 self['context'] and \
                 self['context'].serialize() or \
                 str(self.get(k, '')) for k in self.send ]

class Limits(AbstractReadOperation):
    code = 'LI'
    send = required = 'ip',
    receive = 'ip', 'max_octets_in', 'max_octets_out', \
              'max_bps_in', 'max_bps_out', \
              'max_duration', 'max_idle'
    ints = receive[1:]

class Counters(AbstractReadOperation):
    code = 'CN'
    send = required = 'ip',
    receive = 'ip', 'octets_in', 'octets_out', \
              'bps_in', 'bps_out', \
              'duration', 'idle'
    ints = receive[1:]

class Outer(AbstractReadOperation):
    code = 'OU'
    send = required = 'ip',
    receive = 'ip', 'snat_ip', 'low_snat_port', 'high_snat_port'
    ints = receive[2:]
    ttl = 60

class Inner(AbstractReadOperation):
    code = 'IN'
    send = required = 'snat_ip', 'snat_port'
    receive = 'snat_ip', 'snat_port', 'ip'
    ints = 'snat_port',
    ttl = 60

class Retention(AbstractReadOperation):
    code = 'RT'
    required = 'ip',
    send = 'ip', 'retention'
    receive = 'ip', 'retention'
    ints = 'retention',

# bootstrap

def uamClient(*args):
     raise UamError('UAM client not initialized')

def setupUamClient(conf='/usr/local/etc/sweetspot/sweetuam.conf'):
    global uamClient
    sweetuam.config_load(conf)
    uamClient = sweetuam.UamClient()

