
// Import Python first. This will define _GNU_SOURCE. This is needed to get strdup (and maybe others). We could also define _GNU_SOURCE ourself, but pyconfig.h from Python has troubles then and redeclares some other stuff. So, to just import Python first is the simplest way.
#include <Python.h>
#include <pythread.h>
#include <iostream>
#include "QtApp.hpp"
#include "PyQtGuiObject.hpp"
#include "PythonHelpers.h"
#include "Builders.hpp"


static PyObject* QtGuiObject_alloc(PyTypeObject *type, Py_ssize_t nitems) {
    PyObject *obj;
    const size_t size = _PyObject_VAR_SIZE(type, nitems+1);
    /* note that we need to add one, for the sentinel */
	
    if (PyType_IS_GC(type))
        obj = _PyObject_GC_Malloc(size);
    else
        obj = (PyObject *)PyObject_MALLOC(size);
	
    if (obj == NULL)
        return PyErr_NoMemory();
	
	// This is why we need this custom alloc: To call the C++ constructor.
    memset(obj, '\0', size);
	new ((PyQtGuiObject*) obj) PyQtGuiObject();
	
    if (type->tp_flags & Py_TPFLAGS_HEAPTYPE)
        Py_INCREF(type);
	
    if (type->tp_itemsize == 0)
        PyObject_INIT(obj, type);
    else
        (void) PyObject_INIT_VAR((PyVarObject *)obj, type, nitems);
	
    if (PyType_IS_GC(type))
        _PyObject_GC_TRACK(obj);
    return obj;
}

static void QtGuiObject_dealloc(PyObject* obj) {
	// This is why we need this custom dealloc: To call the C++ destructor.
	((PyQtGuiObject*) obj)->~PyQtGuiObject();
	Py_TYPE(obj)->tp_free(obj);
}

static int QtGuiObject_init(PyObject* self, PyObject* args, PyObject* kwds) {
	return ((PyQtGuiObject*) self)->init(args, kwds);
}

static PyObject* QtGuiObject_getattr(PyObject* self, char* key) {
	return ((PyQtGuiObject*) self)->getattr(key);
}

static int QtGuiObject_setattr(PyObject* self, char* key, PyObject* value) {
	return ((PyQtGuiObject*) self)->setattr(key, value);
}

// http://docs.python.org/2/c-api/typeobj.html

PyTypeObject QtGuiObject_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"QtGuiObject",
	sizeof(PyQtGuiObject),	// basicsize
	0,	// itemsize
	QtGuiObject_dealloc,		/*tp_dealloc*/
	0,                  /*tp_print*/
	QtGuiObject_getattr,		/*tp_getattr*/
	QtGuiObject_setattr,		/*tp_setattr*/
	0,                  /*tp_compare*/
	0,					/*tp_repr*/
	0,                  /*tp_as_number*/
	0,                  /*tp_as_sequence*/
	0,                  /*tp_as_mapping*/
	0,					/*tp_hash */
	0, // tp_call
	0, // tp_str
	0, // tp_getattro
	0, // tp_setattro
	0, // tp_as_buffer
	Py_TPFLAGS_HAVE_CLASS|Py_TPFLAGS_HAVE_WEAKREFS|Py_TPFLAGS_HAVE_GC, // flags
	"QtGuiObject type", // doc
	0, // tp_traverse
	0, // tp_clear
	0, // tp_richcompare
	offsetof(PyQtGuiObject, weakreflist), // weaklistoffset
	0, // iter
	0, // iternext
	0, // methods
	0, //PlayerMembers, // members
	0, // getset
	0, // base
	0, // dict
	0, // descr_get
	0, // descr_set
	offsetof(PyQtGuiObject, __dict__), // dictoffset
	QtGuiObject_init, // tp_init
	QtGuiObject_alloc, // alloc
	PyType_GenericNew, // new
};



PyObject *
guiQt_main(PyObject* self) {
	// This is called from Python and replaces the main() control.
	
	// It might make sense to assert that we are the main thread.
	// However, there is no good cross-platform way to do this (afaik).
	// We could use Python... For now, we just hope that Qt behaves sane.
	// Anyway, on the Python side, we should have called this
	// in the main thread.
	
	int ret = 0;
	Py_BEGIN_ALLOW_THREADS
	QtApp app;
	ret = app.exec();	
	Py_END_ALLOW_THREADS
	
	PyErr_SetObject(PyExc_SystemExit, PyInt_FromLong(ret));
	return NULL;
}


PyObject *
guiQt_quit(PyObject* self) {
	Py_BEGIN_ALLOW_THREADS
	qApp->quit();
	Py_END_ALLOW_THREADS
	Py_INCREF(Py_None);
	return Py_None;
}


PyObject*
guiQt_updateControlMenu(PyObject* self) {
	Py_BEGIN_ALLOW_THREADS
	//[[NSApp delegate] updateControlMenu];
	Py_END_ALLOW_THREADS
	Py_INCREF(Py_None);
	return Py_None;
}


// TODO: make this a macro or dynamic or so...
PyObject*
guiQt_buildControlObject(PyObject* self, PyObject* args) {
	PyObject* control = NULL;
	if(!PyArg_ParseTuple(args, "O:buildControlObject", &control))
		return NULL;
	if(!PyType_IsSubtype(Py_TYPE(control), &QtGuiObject_Type)) {
		PyErr_Format(PyExc_ValueError, "guiQt.buildControlObject: we expect a QtGuiObject");
		return NULL;
	}
	PyQtGuiObject* guiObject = (PyQtGuiObject*) control;
	buildControlObject(guiObject);
	
	Py_INCREF(control);
	return control;
}



static PyMethodDef module_methods[] = {
	{"main",	(PyCFunction)guiQt_main,	METH_NOARGS,	"overtakes main()"},
	{"quit",	(PyCFunction)guiQt_quit,	METH_NOARGS,	"quit application"},
	{"updateControlMenu",	(PyCFunction)guiQt_updateControlMenu,	METH_NOARGS,	""},
	{"buildControlObject",  guiQt_buildControlObject, METH_VARARGS, ""},
	{NULL,				NULL}	/* sentinel */
};

PyDoc_STRVAR(module_doc,
"GUI Cocoa implementation.");


PyMODINIT_FUNC
initguiQt(void)
{
	PyEval_InitThreads(); /* Start the interpreter's thread-awareness */

	if(PyType_Ready(&QtGuiObject_Type) < 0) {
		Py_FatalError("Can't initialize CocoaGuiObject type");
		return;
	}

	PyObject* m = Py_InitModule3("guiQt", module_methods, module_doc);
	if(!m)
		goto fail;
	
	if(PyModule_AddObject(m, "QtGuiObject", (PyObject*) &QtGuiObject_Type) != 0)
		goto fail;

	return;

fail:
	if(PyErr_Occurred())
		PyErr_Print();
	
	Py_FatalError("guiQt module init error");
}