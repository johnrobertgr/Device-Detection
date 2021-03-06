# This file was automatically generated by SWIG (http://www.swig.org).
# Version 3.0.12
#
# Do not make changes to this file unless you know what you are doing--modify
# the SWIG interface file instead.

from sys import version_info as _swig_python_version_info
if _swig_python_version_info >= (2, 7, 0):
    def swig_import_helper():
        import importlib
        pkg = __name__.rpartition('.')[0]
        mname = '.'.join((pkg, '_FiftyOneDegreesTrieV3')).lstrip('.')
        try:
            return importlib.import_module(mname)
        except ImportError:
            return importlib.import_module('_FiftyOneDegreesTrieV3')
    _FiftyOneDegreesTrieV3 = swig_import_helper()
    del swig_import_helper
elif _swig_python_version_info >= (2, 6, 0):
    def swig_import_helper():
        from os.path import dirname
        import imp
        fp = None
        try:
            fp, pathname, description = imp.find_module('_FiftyOneDegreesTrieV3', [dirname(__file__)])
        except ImportError:
            import _FiftyOneDegreesTrieV3
            return _FiftyOneDegreesTrieV3
        try:
            _mod = imp.load_module('_FiftyOneDegreesTrieV3', fp, pathname, description)
        finally:
            if fp is not None:
                fp.close()
        return _mod
    _FiftyOneDegreesTrieV3 = swig_import_helper()
    del swig_import_helper
else:
    import _FiftyOneDegreesTrieV3
del _swig_python_version_info

try:
    _swig_property = property
except NameError:
    pass  # Python < 2.2 doesn't have 'property'.

try:
    import builtins as __builtin__
except ImportError:
    import __builtin__

def _swig_setattr_nondynamic(self, class_type, name, value, static=1):
    if (name == "thisown"):
        return self.this.own(value)
    if (name == "this"):
        if type(value).__name__ == 'SwigPyObject':
            self.__dict__[name] = value
            return
    method = class_type.__swig_setmethods__.get(name, None)
    if method:
        return method(self, value)
    if (not static):
        if _newclass:
            object.__setattr__(self, name, value)
        else:
            self.__dict__[name] = value
    else:
        raise AttributeError("You cannot add attributes to %s" % self)


def _swig_setattr(self, class_type, name, value):
    return _swig_setattr_nondynamic(self, class_type, name, value, 0)


def _swig_getattr(self, class_type, name):
    if (name == "thisown"):
        return self.this.own()
    method = class_type.__swig_getmethods__.get(name, None)
    if method:
        return method(self)
    raise AttributeError("'%s' object has no attribute '%s'" % (class_type.__name__, name))


def _swig_repr(self):
    try:
        strthis = "proxy of " + self.this.__repr__()
    except __builtin__.Exception:
        strthis = ""
    return "<%s.%s; %s >" % (self.__class__.__module__, self.__class__.__name__, strthis,)

try:
    _object = object
    _newclass = 1
except __builtin__.Exception:
    class _object:
        pass
    _newclass = 0

class SwigPyIterator(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, SwigPyIterator, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, SwigPyIterator, name)

    def __init__(self, *args, **kwargs):
        raise AttributeError("No constructor defined - class is abstract")
    __repr__ = _swig_repr
    __swig_destroy__ = _FiftyOneDegreesTrieV3.delete_SwigPyIterator
    __del__ = lambda self: None

    def value(self):
        return _FiftyOneDegreesTrieV3.SwigPyIterator_value(self)

    def incr(self, n=1):
        return _FiftyOneDegreesTrieV3.SwigPyIterator_incr(self, n)

    def decr(self, n=1):
        return _FiftyOneDegreesTrieV3.SwigPyIterator_decr(self, n)

    def distance(self, x):
        return _FiftyOneDegreesTrieV3.SwigPyIterator_distance(self, x)

    def equal(self, x):
        return _FiftyOneDegreesTrieV3.SwigPyIterator_equal(self, x)

    def copy(self):
        return _FiftyOneDegreesTrieV3.SwigPyIterator_copy(self)

    def next(self):
        return _FiftyOneDegreesTrieV3.SwigPyIterator_next(self)

    def __next__(self):
        return _FiftyOneDegreesTrieV3.SwigPyIterator___next__(self)

    def previous(self):
        return _FiftyOneDegreesTrieV3.SwigPyIterator_previous(self)

    def advance(self, n):
        return _FiftyOneDegreesTrieV3.SwigPyIterator_advance(self, n)

    def __eq__(self, x):
        return _FiftyOneDegreesTrieV3.SwigPyIterator___eq__(self, x)

    def __ne__(self, x):
        return _FiftyOneDegreesTrieV3.SwigPyIterator___ne__(self, x)

    def __iadd__(self, n):
        return _FiftyOneDegreesTrieV3.SwigPyIterator___iadd__(self, n)

    def __isub__(self, n):
        return _FiftyOneDegreesTrieV3.SwigPyIterator___isub__(self, n)

    def __add__(self, n):
        return _FiftyOneDegreesTrieV3.SwigPyIterator___add__(self, n)

    def __sub__(self, *args):
        return _FiftyOneDegreesTrieV3.SwigPyIterator___sub__(self, *args)
    def __iter__(self):
        return self
SwigPyIterator_swigregister = _FiftyOneDegreesTrieV3.SwigPyIterator_swigregister
SwigPyIterator_swigregister(SwigPyIterator)

class MapStringString(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, MapStringString, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, MapStringString, name)
    __repr__ = _swig_repr

    def iterator(self):
        return _FiftyOneDegreesTrieV3.MapStringString_iterator(self)
    def __iter__(self):
        return self.iterator()

    def __nonzero__(self):
        return _FiftyOneDegreesTrieV3.MapStringString___nonzero__(self)

    def __bool__(self):
        return _FiftyOneDegreesTrieV3.MapStringString___bool__(self)

    def __len__(self):
        return _FiftyOneDegreesTrieV3.MapStringString___len__(self)
    def __iter__(self):
        return self.key_iterator()
    def iterkeys(self):
        return self.key_iterator()
    def itervalues(self):
        return self.value_iterator()
    def iteritems(self):
        return self.iterator()

    def __getitem__(self, key):
        return _FiftyOneDegreesTrieV3.MapStringString___getitem__(self, key)

    def __delitem__(self, key):
        return _FiftyOneDegreesTrieV3.MapStringString___delitem__(self, key)

    def has_key(self, key):
        return _FiftyOneDegreesTrieV3.MapStringString_has_key(self, key)

    def keys(self):
        return _FiftyOneDegreesTrieV3.MapStringString_keys(self)

    def values(self):
        return _FiftyOneDegreesTrieV3.MapStringString_values(self)

    def items(self):
        return _FiftyOneDegreesTrieV3.MapStringString_items(self)

    def __contains__(self, key):
        return _FiftyOneDegreesTrieV3.MapStringString___contains__(self, key)

    def key_iterator(self):
        return _FiftyOneDegreesTrieV3.MapStringString_key_iterator(self)

    def value_iterator(self):
        return _FiftyOneDegreesTrieV3.MapStringString_value_iterator(self)

    def __setitem__(self, *args):
        return _FiftyOneDegreesTrieV3.MapStringString___setitem__(self, *args)

    def asdict(self):
        return _FiftyOneDegreesTrieV3.MapStringString_asdict(self)

    def __init__(self, *args):
        this = _FiftyOneDegreesTrieV3.new_MapStringString(*args)
        try:
            self.this.append(this)
        except __builtin__.Exception:
            self.this = this

    def empty(self):
        return _FiftyOneDegreesTrieV3.MapStringString_empty(self)

    def size(self):
        return _FiftyOneDegreesTrieV3.MapStringString_size(self)

    def swap(self, v):
        return _FiftyOneDegreesTrieV3.MapStringString_swap(self, v)

    def begin(self):
        return _FiftyOneDegreesTrieV3.MapStringString_begin(self)

    def end(self):
        return _FiftyOneDegreesTrieV3.MapStringString_end(self)

    def rbegin(self):
        return _FiftyOneDegreesTrieV3.MapStringString_rbegin(self)

    def rend(self):
        return _FiftyOneDegreesTrieV3.MapStringString_rend(self)

    def clear(self):
        return _FiftyOneDegreesTrieV3.MapStringString_clear(self)

    def get_allocator(self):
        return _FiftyOneDegreesTrieV3.MapStringString_get_allocator(self)

    def count(self, x):
        return _FiftyOneDegreesTrieV3.MapStringString_count(self, x)

    def erase(self, *args):
        return _FiftyOneDegreesTrieV3.MapStringString_erase(self, *args)

    def find(self, x):
        return _FiftyOneDegreesTrieV3.MapStringString_find(self, x)

    def lower_bound(self, x):
        return _FiftyOneDegreesTrieV3.MapStringString_lower_bound(self, x)

    def upper_bound(self, x):
        return _FiftyOneDegreesTrieV3.MapStringString_upper_bound(self, x)
    __swig_destroy__ = _FiftyOneDegreesTrieV3.delete_MapStringString
    __del__ = lambda self: None
MapStringString_swigregister = _FiftyOneDegreesTrieV3.MapStringString_swigregister
MapStringString_swigregister(MapStringString)

class VectorString(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, VectorString, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, VectorString, name)
    __repr__ = _swig_repr

    def iterator(self):
        return _FiftyOneDegreesTrieV3.VectorString_iterator(self)
    def __iter__(self):
        return self.iterator()

    def __nonzero__(self):
        return _FiftyOneDegreesTrieV3.VectorString___nonzero__(self)

    def __bool__(self):
        return _FiftyOneDegreesTrieV3.VectorString___bool__(self)

    def __len__(self):
        return _FiftyOneDegreesTrieV3.VectorString___len__(self)

    def __getslice__(self, i, j):
        return _FiftyOneDegreesTrieV3.VectorString___getslice__(self, i, j)

    def __setslice__(self, *args):
        return _FiftyOneDegreesTrieV3.VectorString___setslice__(self, *args)

    def __delslice__(self, i, j):
        return _FiftyOneDegreesTrieV3.VectorString___delslice__(self, i, j)

    def __delitem__(self, *args):
        return _FiftyOneDegreesTrieV3.VectorString___delitem__(self, *args)

    def __getitem__(self, *args):
        return _FiftyOneDegreesTrieV3.VectorString___getitem__(self, *args)

    def __setitem__(self, *args):
        return _FiftyOneDegreesTrieV3.VectorString___setitem__(self, *args)

    def pop(self):
        return _FiftyOneDegreesTrieV3.VectorString_pop(self)

    def append(self, x):
        return _FiftyOneDegreesTrieV3.VectorString_append(self, x)

    def empty(self):
        return _FiftyOneDegreesTrieV3.VectorString_empty(self)

    def size(self):
        return _FiftyOneDegreesTrieV3.VectorString_size(self)

    def swap(self, v):
        return _FiftyOneDegreesTrieV3.VectorString_swap(self, v)

    def begin(self):
        return _FiftyOneDegreesTrieV3.VectorString_begin(self)

    def end(self):
        return _FiftyOneDegreesTrieV3.VectorString_end(self)

    def rbegin(self):
        return _FiftyOneDegreesTrieV3.VectorString_rbegin(self)

    def rend(self):
        return _FiftyOneDegreesTrieV3.VectorString_rend(self)

    def clear(self):
        return _FiftyOneDegreesTrieV3.VectorString_clear(self)

    def get_allocator(self):
        return _FiftyOneDegreesTrieV3.VectorString_get_allocator(self)

    def pop_back(self):
        return _FiftyOneDegreesTrieV3.VectorString_pop_back(self)

    def erase(self, *args):
        return _FiftyOneDegreesTrieV3.VectorString_erase(self, *args)

    def __init__(self, *args):
        this = _FiftyOneDegreesTrieV3.new_VectorString(*args)
        try:
            self.this.append(this)
        except __builtin__.Exception:
            self.this = this

    def push_back(self, x):
        return _FiftyOneDegreesTrieV3.VectorString_push_back(self, x)

    def front(self):
        return _FiftyOneDegreesTrieV3.VectorString_front(self)

    def back(self):
        return _FiftyOneDegreesTrieV3.VectorString_back(self)

    def assign(self, n, x):
        return _FiftyOneDegreesTrieV3.VectorString_assign(self, n, x)

    def resize(self, *args):
        return _FiftyOneDegreesTrieV3.VectorString_resize(self, *args)

    def insert(self, *args):
        return _FiftyOneDegreesTrieV3.VectorString_insert(self, *args)

    def reserve(self, n):
        return _FiftyOneDegreesTrieV3.VectorString_reserve(self, n)

    def capacity(self):
        return _FiftyOneDegreesTrieV3.VectorString_capacity(self)
    __swig_destroy__ = _FiftyOneDegreesTrieV3.delete_VectorString
    __del__ = lambda self: None
VectorString_swigregister = _FiftyOneDegreesTrieV3.VectorString_swigregister
VectorString_swigregister(VectorString)

class Match(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, Match, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, Match, name)

    def __init__(self, *args, **kwargs):
        raise AttributeError("No constructor defined")
    __repr__ = _swig_repr
    __swig_destroy__ = _FiftyOneDegreesTrieV3.delete_Match
    __del__ = lambda self: None

    def getValues(self, *args):
        return _FiftyOneDegreesTrieV3.Match_getValues(self, *args)

    def getValue(self, *args):
        return _FiftyOneDegreesTrieV3.Match_getValue(self, *args)

    def getDeviceId(self):
        return _FiftyOneDegreesTrieV3.Match_getDeviceId(self)

    def getRank(self):
        return _FiftyOneDegreesTrieV3.Match_getRank(self)

    def getDifference(self):
        return _FiftyOneDegreesTrieV3.Match_getDifference(self)

    def getMethod(self):
        return _FiftyOneDegreesTrieV3.Match_getMethod(self)

    def getUserAgent(self):
        return _FiftyOneDegreesTrieV3.Match_getUserAgent(self)
Match_swigregister = _FiftyOneDegreesTrieV3.Match_swigregister
Match_swigregister(Match)

class Provider(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, Provider, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, Provider, name)
    __repr__ = _swig_repr
    __swig_destroy__ = _FiftyOneDegreesTrieV3.delete_Provider
    __del__ = lambda self: None

    def getHttpHeaders(self):
        return _FiftyOneDegreesTrieV3.Provider_getHttpHeaders(self)

    def getAvailableProperties(self):
        return _FiftyOneDegreesTrieV3.Provider_getAvailableProperties(self)

    def getDataSetName(self):
        return _FiftyOneDegreesTrieV3.Provider_getDataSetName(self)

    def getDataSetFormat(self):
        return _FiftyOneDegreesTrieV3.Provider_getDataSetFormat(self)

    def getDataSetPublishedDate(self):
        return _FiftyOneDegreesTrieV3.Provider_getDataSetPublishedDate(self)

    def getDataSetNextUpdateDate(self):
        return _FiftyOneDegreesTrieV3.Provider_getDataSetNextUpdateDate(self)

    def getDataSetSignatureCount(self):
        return _FiftyOneDegreesTrieV3.Provider_getDataSetSignatureCount(self)

    def getDataSetDeviceCombinations(self):
        return _FiftyOneDegreesTrieV3.Provider_getDataSetDeviceCombinations(self)

    def getMatch(self, *args):
        return _FiftyOneDegreesTrieV3.Provider_getMatch(self, *args)

    def getMatchWithTolerances(self, *args):
        return _FiftyOneDegreesTrieV3.Provider_getMatchWithTolerances(self, *args)

    def getMatchJson(self, *args):
        return _FiftyOneDegreesTrieV3.Provider_getMatchJson(self, *args)

    def setDrift(self, drift):
        return _FiftyOneDegreesTrieV3.Provider_setDrift(self, drift)

    def setDifference(self, difference):
        return _FiftyOneDegreesTrieV3.Provider_setDifference(self, difference)

    def reloadFromFile(self):
        return _FiftyOneDegreesTrieV3.Provider_reloadFromFile(self)

    def reloadFromMemory(self, source, size):
        return _FiftyOneDegreesTrieV3.Provider_reloadFromMemory(self, source, size)

    def getIsThreadSafe(self):
        return _FiftyOneDegreesTrieV3.Provider_getIsThreadSafe(self)

    def __init__(self, *args):
        this = _FiftyOneDegreesTrieV3.new_Provider(*args)
        try:
            self.this.append(this)
        except __builtin__.Exception:
            self.this = this
Provider_swigregister = _FiftyOneDegreesTrieV3.Provider_swigregister
Provider_swigregister(Provider)

# This file is compatible with both classic and new-style classes.


