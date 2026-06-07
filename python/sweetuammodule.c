/*
** Python extention module for Sweetspot UAM client.
**
** Written by Alexey Michurin <am@rol.ru>, 2008, 2014.
*/

#include <string.h>
#include <stdio.h>

#include <Python.h>

#include "structmember.h"

#include "config.h"
#include "uammsg.h"
#include "uamclt.h"
#include "debug.h"
#include "cfg.h"
#include "log.h"

#define UAM_ARGS_BUFFER_LENGTH 4096

static int global_config_loaded = 0;

/*
 *  DEFS
 */

static PyObject *uam_error;

typedef struct {
    PyObject_HEAD
    sw_uamclt_group_t * sweetuam_group;
} sweetuamObject;

#ifndef HAVE_PY_SSIZE_T
typedef int Py_ssize_t;  /* Py_ssize_t use in python 2.5 */
#endif

static void       sweetuam_dealloc         (sweetuamObject *self);
static int        sweetuam_init            (sweetuamObject *self, PyObject *args, PyObject *kwds);

static PyObject * sweetuam_set_debug_flags (PyObject *self, PyObject *args);
static PyObject * sweetuam_config_load     (PyObject *self, PyObject *args);
static PyObject * sweetuam_config_unload   (PyObject *self);
static PyObject * sweetuam_send_recv       (sweetuamObject *self, PyObject *args);
static PyObject * sweetuam_fileno          (sweetuamObject *self);
static PyObject * sweetuam_send            (sweetuamObject *self, PyObject *args);
static PyObject * sweetuam_recv            (sweetuamObject *self);

static PyMemberDef sweetuam_members[] = {
    { NULL }  /* Sentinel */
};

static PyMethodDef sweetuam_methods[] = {
    { "fileno", (PyCFunction)sweetuam_fileno, METH_NOARGS,
      "x.fileno() -> Return the integer file descriptor of the socket." },
    { "sendMsg", (PyCFunction)sweetuam_send, METH_VARARGS,
      "x.sendMsg(*args)\nSend UAM message (composed of string-typed tokens) to server." },
    { "recvMsg", (PyCFunction)sweetuam_recv, METH_NOARGS,
      "rep = x.recvMsg()\nReceive UAM message from server, return response as a tuple of string-typed tokens." },
    { NULL, NULL, 0, NULL }  /* Sentinel */
};

static PyMethodDef sweetuam_functions[] = {
    { "set_debug_flags", (PyCFunction)sweetuam_set_debug_flags, METH_VARARGS,
      "set_debug_flags(flag1, flag2,...)\n\n"
      "Flags: io, snat, dnat, filter, uam, acct, session, relay, tuple,\n"
      "       netset, frag, core, relations, gauge, breath, all.\n"
      "Return number of arguments.\n"
      "If flag not valid the ValueError raised."  },
    { "config_load", (PyCFunction)sweetuam_config_load, METH_VARARGS,
      "config_load(filename)\nconfig_load()\n\n"
      "Load configuration file or raise IOError exception.\n"
      "If file not specified, try to load configuration from\n"
      "default place.\n"
      "Return filename." },
    { "config_unload", (PyCFunction)sweetuam_config_unload, METH_NOARGS,
      "config_unload()\n\n"
      "Unload configuration file or raise IOError exception.\n"
      "Return None." },
    { NULL, NULL, 0, NULL }  /* Sentinel */
};


static PyTypeObject sweetuamType = {
    PyObject_HEAD_INIT(NULL)
    0,                     /* ob_size (not used) */
    "UamClient",           /* tp_name */
    sizeof(sweetuamObject),   /* tp_basicsize */
    0,                     /* tp_itemsize */
    /* Methods to implement standard operations */
    (destructor)sweetuam_dealloc, /* tp_dealloc */
    0,                     /* tp_print */
    0,                     /* tp_getattr */
    0,                     /* tp_setattr */
    0,                     /* tp_compare */
    0,                     /* tp_repr */
    /* Method suites for standard classes */
    0,                     /* tp_as_number */
    0,                     /* tp_as_sequence */
    0,                     /* tp_as_mapping */
    /* More standard operations (here for binary compatibility? or not only?) */
    0,                     /* tp_hash */
    (ternaryfunc)sweetuam_send_recv,    /* tp_call */
    0,                     /* tp_str */
    0,                     /* tp_getattro */
    0,                     /* tp_setattro */
    /* Functions to access object as input/output buffer */
    0,                     /* tp_as_buffer */
    /* Flags to define presence of optional/expanded features */
    Py_TPFLAGS_DEFAULT,    /* tp_flags */
    /* Documentation string */
    "uam_client = UamClient()\n"
    "reply_tuple = uam_client('UP', '192.164.0.1')\n"
    "del uam_client\n\n"
    "Create object to communicate with sweetspot daemon.\n\n"
    "NOTE: You must load configuration file *before*\n"
    "      you create a client object.",    /* tp_doc */
    /* Assigned meaning in release 2.0 */
    /* call function for all accessible objects */
    0,                     /* tp_traverse */
    /* delete references to contained objects */
    0,                     /* tp_clear */
    /* Assigned meaning in release 2.1 */
    /* rich comparisons */
    0,                     /* tp_richcompare */
    /* weak reference enabler */
    0,                     /* tp_weaklistoffset */
    /* Added in release 2.2 */
    /* Iterators */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    /* Attribute descriptor and subclassing stuff */
    sweetuam_methods,             /* tp_methods */
    sweetuam_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)sweetuam_init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
    0,                         /* tp_free; Low-level free-memory routine */
    0,                         /* tp_is_gc; For PyObject_IS_GC */
    0,                         /* tp_bases */
    0,                         /* tp_mro (method resolution order) */
    0,                         /* tp_cache */
    0,                         /* tp_subclasses */
    0                          /* tp_weaklist */
};

/*
 *  FUNCTIONS
 */

static PyObject * sweetuam_set_debug_flags(PyObject *self, PyObject *args) {
    const char *s;
    Py_ssize_t l, i;
    PyObject *a;
    int rc;
    l = PySequence_Length(args);
    for (i = 0; i < l; i++) {
        a = PySequence_GetItem(args, i);
        s = PyString_AsString(a); /* do not modify data! */
        rc = sw_debug_create(s);
        Py_DECREF(a);
        if (rc == -1) {
          PyErr_SetString(PyExc_ValueError, "invalid debug flag found");
          return NULL;
        }
    }
    return PyInt_FromLong((long)l); /* use PyInt_FromSsize_t(l); in Py 2.5 */
}

static PyObject * sweetuam_config_load(PyObject *self, PyObject *args) {
    static char *config_filename = SW_SWEETUAM_CONFIG_FILE;
    if (global_config_loaded) {
        PyErr_SetString(uam_error, "config file already loaded");
        return NULL;
    }
    if (PySequence_Length(args)) {
        if (!PyArg_ParseTuple(args, "s", &config_filename)) {
            /* Exception object ready now */
            return NULL;
        }
    }
    if (sw_config_load(config_filename) < 0) {
        PyErr_SetString(uam_error, "sw_config_load() failed");
        return NULL;
    }
    if (sw_debug_flags & SW_DEBUG_UAM)
        sw_log("%s:%d config %s loaded", __FILE__, __LINE__, config_filename);
    global_config_loaded = 1;
    return PyString_FromString(config_filename);
}

static PyObject * sweetuam_config_unload(PyObject *self) {
    if (!global_config_loaded) {
        PyErr_SetString(uam_error, "config file not loaded");
        return NULL;
    }
    if (sw_config_unload() < 0) {
        PyErr_SetString(uam_error, "sw_config_unload() failed");
        return NULL;
    }
    if (sw_debug_flags & SW_DEBUG_UAM)
        sw_log("%s:%d config unloaded", __FILE__, __LINE__);
    global_config_loaded = 0;
    Py_INCREF(Py_None);
    return Py_None;
}

/*
 *  METHODS
 */

/* Constructor / Destructor */

static int sweetuam_init(sweetuamObject *self, PyObject *args, PyObject *kwds) {
    self->sweetuam_group = NULL;
    if (!global_config_loaded) {
        PyErr_SetString(uam_error, "config file is not loaded yet");
        return -1;
    }
    if (sw_uam_create_client(&(self->sweetuam_group)) == -1) {
        PyErr_SetString(uam_error, "sw_uam_create_client() failed");
        return -1;
    }
    if (sw_debug_flags & SW_DEBUG_UAM)
        sw_log("%s:%d init object id=%p", __FILE__, __LINE__, self);
    return 0;
}

static void sweetuam_dealloc(sweetuamObject* self) {
    if (self->sweetuam_group)
        sw_uam_destroy_client(self->sweetuam_group);
    self->ob_type->tp_free((PyObject*)self); /* important! */
    if (sw_debug_flags & SW_DEBUG_UAM)
        sw_log("%s:%d destroy object id=%p", __FILE__, __LINE__, self);
}

/* Util */

int send_message(sweetuamObject *self, PyObject *args) {
    char uam_args_buffer[UAM_ARGS_BUFFER_LENGTH];
    char *uam_buff_ptr;
    char *py_str_ptr;
    Py_ssize_t py_str_len;
    char *sw_argv[SW_UAM_ARG_MAX];
    Py_ssize_t l, i;
    PyObject *a;
    int rc;
    l = PySequence_Length(args);
    if (l >= SW_UAM_ARG_MAX) {
        PyErr_SetString(PyExc_ValueError, "too many UAM arguments");
        return 0;
    }
    uam_buff_ptr = uam_args_buffer;
    for (i = 0; i < l; i++) {
        a = PySequence_GetItem(args, i);
        if (a) {
            rc = PyString_AsStringAndSize(a, &py_str_ptr, &py_str_len); /* do not modify data!!! */
            if (!rc) {
                /*
                printf("s = %d rc = %d s='%.*s'\n", py_str_len, rc,  py_str_len,  py_str_ptr);
                */
                py_str_len++;
                if (uam_buff_ptr + py_str_len > uam_buff_ptr + UAM_ARGS_BUFFER_LENGTH) {
                    PyErr_SetString(PyExc_ValueError, "uam args buffer owerflow");
                    rc = -1;
                } else {
                    memcpy(uam_buff_ptr, py_str_ptr, py_str_len);
                    sw_argv[i] = uam_buff_ptr;
                    uam_buff_ptr += py_str_len;
                }
            }
            Py_DECREF(a);
            if (rc) {
                return 0;
            }
        } else {
            PyErr_SetString(PyExc_ValueError, "can not ge element");
            return 0;
        }
    }
    for(i = l; i < SW_UAM_ARG_MAX; i++) {
        sw_argv[i] = NULL;
    }
/*
    for(i = 0; i < SW_UAM_ARG_MAX; i++) {
        printf("RES: %d: <%s>\n", i, sw_argv[i]);
    }
*/
    if (sw_uam_send_msg(self->sweetuam_group, sw_argv) == -1) {
        PyErr_SetString(uam_error, "sw_uam_send_msg() failed");
        return 0;
    }
    if (sw_debug_flags & (SW_DEBUG_UAM | SW_DEBUG_BREATH))
        sw_log("%s:%d uam message issued", __FILE__, __LINE__);
    return 1;
}

static PyObject * create_tuple(char *sw_argv[]) {
    Py_ssize_t i;
    PyObject *a;
/*
    printf("--RESULT:\n");
    for(i = 0; sw_argv[i]; i++)
        printf("%s\n", *sw_argv[i] ? sw_argv[i] : "<EMPTY>");
    printf("--\n");
*/
    for(i = 0; sw_argv[i]; i++);
    a = PyTuple_New(i);
    for(i = 0; sw_argv[i]; i++)
        PyTuple_SetItem(a, i, PyString_FromString(sw_argv[i]));
    return a;
}

/* Python interface */

static PyObject * sweetuam_send(sweetuamObject *self, PyObject *args) {
    if (!send_message(self, args))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * sweetuam_recv(sweetuamObject *self) {
    char *sw_argv[SW_UAM_ARG_MAX];
    char cyphertext[SW_UAM_MSG_SIZE];
    int rc;

    rc = recv(self->sweetuam_group->sock,
              &cyphertext, sizeof(cyphertext), 0);
    if (rc == -1) {
        sw_log("%s:%d recv() failed: %s", __FILE__, __LINE__, strerror(errno));
        PyErr_SetString(uam_error, "client recv failure");
        return NULL;
    }

    if (sw_debug_flags & SW_DEBUG_UAM)
        sw_log("%s:%d recved UAM msg: %d octets", __FILE__, __LINE__, rc);

    rc = sw_uam_parse_msg(sw_argv, cyphertext, rc);
    if (rc == -1) {
        PyErr_SetString(uam_error, "sw_uam_parse_msg() failed");
        return NULL;
    }
    return create_tuple(sw_argv);
}

static PyObject * sweetuam_send_recv(sweetuamObject *self, PyObject *args) {
    char *sw_argv[SW_UAM_ARG_MAX];

    if (!send_message(self, args)) {
        return NULL;
    }
    if (sw_uam_recv_msg(self->sweetuam_group, sw_argv) == -1) {
        PyErr_SetString(uam_error, "sw_uam_recv_msg() failed");
        return NULL;
    }
    if (sw_debug_flags & (SW_DEBUG_UAM | SW_DEBUG_BREATH))
        sw_log("%s:%d UAM message received", __FILE__, __LINE__);
    return create_tuple(sw_argv);
}

static PyObject * sweetuam_fileno(sweetuamObject *self) {
    return PyInt_FromLong((long)(self->sweetuam_group->sock));
}

/*
 *  INIT MODULE
 */

PyMODINIT_FUNC initsweetuam(void) {
    PyObject* m;

    uam_error = PyErr_NewException("sweetuam.UAMError", PyExc_IOError, NULL);

    sweetuamType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&sweetuamType) < 0) {
        return;
    }

    m = Py_InitModule3("sweetuam", sweetuam_functions,
                       "# Sweetspot UAM client.\n\n"
                       "import sweetuam\n"
                       "# sweetuam.set_debug_flags('all')\n"
                       "sweetuam.config_load()\n"
                       "# sync API\n"
                       "uamClient = sweetuam.UamClient()\n"
                       "print uamClient('UP', '192.168.102.1')\n"
                       "del uamClient\n\n"
                       "# async API\n"
                       "uamClient = sweetuam.UamClient()\n"
                       "fn = uamClient.fileno()\n"
                       "uamClient.sendMsg('UP', '192.168.102.1')\n"
                       "select.select([fn], [], [])\n"
                       "print uamClient.recvMsg()\n"
                       "del uamClient\n\n"
               );

    Py_INCREF(&sweetuamType);
    PyModule_AddObject(m, "UamClient", (PyObject *)&sweetuamType);
    PyModule_AddObject(m, "UAMError", uam_error);
    PyModule_AddStringConstant(m, "__version__", SW_MODULE_VERSION);
}
