#ifndef CAPSPLUG_H
#define CAPSPLUG_H

#ifdef __cplusplus

// library function pointers
struct CapsProc {
	LPCSTR name;
	FARPROC proc;
};

typedef SDWORD (__cdecl *CAPSHOOKN)(...);
typedef PCHAR  (__cdecl *CAPSHOOKS)(...);

extern "C" {
#endif

SDWORD CapsInit(LPCTSTR lib);
SDWORD CapsExit();
SDWORD CapsAddImage();
SDWORD CapsRemImage(SDWORD id);
SDWORD CapsLockImage(SDWORD id, PCHAR name);
SDWORD CapsLockImageMemory(SDWORD id, PUBYTE buffer, UDWORD length, UDWORD flag);
SDWORD CapsUnlockImage(SDWORD id);
SDWORD CapsLoadImage(SDWORD id, UDWORD flag);
SDWORD CapsGetImageInfo(PCAPSIMAGEINFO pi, SDWORD id);
SDWORD CapsLockTrack(PCAPSTRACKINFO pi, SDWORD id, UDWORD cylinder, UDWORD head, UDWORD flag);
SDWORD CapsUnlockTrack(SDWORD id, UDWORD cylinder, UDWORD head);
SDWORD CapsUnlockAllTracks(SDWORD id);
PCHAR  CapsGetPlatformName(UDWORD pid);

#ifdef __cplusplus
}
#endif

#endif
