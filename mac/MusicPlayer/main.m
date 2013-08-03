//
//  main.m
//  MusicPlayer
//
//  Created by Albert Zeyer on 21.08.12.
//  Copyright (c) 2012 Albert Zeyer. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Python/Python.h>



static void addPyPath() {
	NSString* pathStr =
	[[NSString alloc]
	 initWithFormat:@"%s:%s%s:%@/Python:%s",
	 Py_GetPath(),
	 Py_GetPrefix(), "/Extras/lib/python/PyObjC",
	 [[NSBundle mainBundle] resourcePath],
	 "/System/Library/Frameworks/Python.framework/Versions/Current/Extras/lib/python/PyObjC"
	 ];
	PySys_SetPath((char*)[pathStr UTF8String]);
	NSLog(@"Python path: %@", pathStr);
	[pathStr release];
}

static int sys_argc;
static char** sys_argv;

static bool haveArg(const char* arg) {
	for(int i = 1; i < sys_argc; ++i)
		if(strcmp(sys_argv[i], arg) == 0) {
			return true;
		}
	return false;
}

int main(int argc, char *argv[])
{
	sys_argc = argc;
	sys_argv = argv;
	//return NSApplicationMain(argc, (const char **)argv);

	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	bool forkExecProc = haveArg("--forkExecProc");
	bool shell = haveArg("--shell");
	bool pyShell = haveArg("--pyshell");
	
	NSString* mainPyFilename = [[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/Python/main.py"];
	Py_SetProgramName(argv[0]);
	if(!forkExecProc)
		NSLog(@"Python version: %s, prefix: %s, main: %@", Py_GetVersion(), Py_GetPrefix(), mainPyFilename);
	
	Py_Initialize();
	PyEval_InitThreads();
	addPyPath();

	// Preload imp and thread. I hope to fix this bug: https://github.com/albertz/music-player/issues/8 , there was a crash in initthread which itself has called initimp
	PyObject* m = NULL;
	m = PyImport_ImportModule("imp");
	Py_XDECREF(m);
	m = PyImport_ImportModule("thread");
	Py_XDECREF(m);
	
	PySys_SetArgvEx(argc, argv, 0);
	
	if(!forkExecProc && !pyShell && !shell) {
		// current workaround to log stdout/stderr. see http://stackoverflow.com/questions/13104588/how-to-get-stdout-into-console-app
		printf("stdout/stderr goes to ~/Library/Logs/com.albertzeyer.MusicPlayer.log now\n");
		freopen([[@"~/Library/Logs/com.albertzeyer.MusicPlayer.log" stringByExpandingTildeInPath] UTF8String], "a", stdout);
		freopen([[@"~/Library/Logs/com.albertzeyer.MusicPlayer.log" stringByExpandingTildeInPath] UTF8String], "a", stderr);
		PyRun_SimpleString("print 'hello there'");
	}
	
	FILE* fp = fopen((char*)[mainPyFilename UTF8String], "r");
	assert(fp);
	PyRun_SimpleFile(fp, "main.py");
	
	[pool release];
	return 0;
}
