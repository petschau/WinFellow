/* FELLOW IN START */

#include <windows.h>
#include <tchar.h>

#define PATHPREFIX "\\\\?\\"
#define MAX_DPATH 1000

struct uae_prefs {
  bool win32_filesystem_mangle_reserved_names;
} currprefs = { true };

/* FELLOW IN END */

int my_rename(const TCHAR* oldname, const TCHAR* newname)
{
  const TCHAR* onamep, * nnamep;
  TCHAR opath[MAX_DPATH], npath[MAX_DPATH];

  if (currprefs.win32_filesystem_mangle_reserved_names == false) {
    _tcscpy(opath, PATHPREFIX);
    _tcscat(opath, oldname);
    onamep = opath;
    _tcscpy(npath, PATHPREFIX);
    _tcscat(npath, newname);
    nnamep = npath;
  }
  else {
    onamep = oldname;
    nnamep = newname;
  }
  return MoveFile(onamep, nnamep) == 0 ? -1 : 0;
}