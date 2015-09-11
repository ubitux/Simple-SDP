// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LIBSIMPLESDP_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LIBSIMPLESDP_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef LIBSIMPLESDP_EXPORTS
#define LIBSIMPLESDP_API __declspec(dllexport)
#else
#define LIBSIMPLESDP_API __declspec(dllimport)
#endif

// This class is exported from the libSimpleSDP.dll
class LIBSIMPLESDP_API ClibSimpleSDP {
public:
	ClibSimpleSDP(void);
	// TODO: add your methods here.
};

extern LIBSIMPLESDP_API int nlibSimpleSDP;

LIBSIMPLESDP_API int fnlibSimpleSDP(void);
