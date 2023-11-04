/*
 * UAE - The Un*x Amiga Emulator
 *
 * Library of functions to make emulated filesystem as independent as
 * possible of the host filesystem's capabilities.
 *
 * Copyright 1999 Bernd Schmidt
 */

/* FELLOW IN (START)-----------------

  This file has been adapted for use in WinFellow.
  It originates from the UAE 0.8.22 source code distribution.

  Torsten Enderling (carfesh@gmx.net) 2004

   FELLOW IN (END)------------------- */

/* FELLOW OUT (START)-------------------
#include "sysconfig.h"
#include "sysdeps.h"

#include "Configuration.h"
#include "threaddep/thread.h"
#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "filesys.h"
#include "autoconf.h"
#include "compiler.h"
#include "fsusage.h"
#include "native2amiga.h"
#include "scsidev.h"
   FELLOW OUT (END)------------------ */

#include "fsdb.h"
#ifdef WIN32
#include "filesys_win32.h"
#endif

/* The on-disk format is as follows:
 * Offset 0, 1 byte, valid
 * Offset 1, 4 bytes, mode
 * Offset 5, 257 bytes, aname
 * Offset 263, 257 bytes, nname
 * Offset 518, 81 bytes, comment
 */

char *nname_begin(char *nname)
{
  char *p = strrchr(nname, FSDB_DIR_SEPARATOR);
  if (p) return p + 1;
  return nname;
}

/* Find the name REL in directory DIRNAME.  If we find a file that
 * has exactly the same name, return REL.  If we find a file that
 * has the same name when compared case-insensitively, return a
 * malloced string that contains the name we found.  If no file
 * exists that compares equal to REL, return 0.  */
char *fsdb_search_dir(const char *dirname, char *rel)
{
  char *p = nullptr;
  struct dirent *de;
  DIR *dir = opendir(dirname);

  /* This really shouldn't happen...  */
  if (!dir) return nullptr;

  while (p == nullptr && (de = readdir(dir)) != nullptr)
  {
    if (strcmp(de->d_name, rel) == 0)
      p = rel;
    else if (strcasecmp(de->d_name, rel) == 0)
      p = my_strdup(de->d_name);
  }
  closedir(dir);
  return p;
}

static FILE *get_fsdb(a_inode *dir, const char *mode)
{
  char *n = build_nname(dir->nname, FSDB_FILE);
  FILE *f = fopen(n, mode);
  free(n);
  return f;
}

static void kill_fsdb(a_inode *dir)
{
  char *n = build_nname(dir->nname, FSDB_FILE);
  unlink(n);
  free(n);
}

/* Prune the db file the first time this directory is opened in a session.  */
void fsdb_clean_dir(a_inode *dir)
{
  char buf[1 + 4 + 257 + 257 + 81];
  char *n = build_nname(dir->nname, FSDB_FILE);
  FILE *f = fopen(n, "r+b");
  off_t pos1 = 0, pos2 = 0;

  /* FELLOW BUGFIX (START) : if (f == 0) return;*/
  if (f == nullptr)
  {
    free(n); /* leak */
    return;
  }
  /* FELLOW BUGFIX (END) */
  for (;; pos2 += sizeof buf)
  {
    if (fread(buf, 1, sizeof buf, f) < sizeof buf) break;
    if (buf[0] == 0) continue;
    if (pos1 != pos2)
    {
      fseek(f, pos1, SEEK_SET);
      fwrite(buf, 1, sizeof buf, f);
      fseek(f, pos2 + sizeof buf, SEEK_SET);
    }
    pos1 += sizeof buf;
  }
  fclose(f);
  truncate(n, pos1);
  free(n);
}

static a_inode *aino_from_buf(a_inode *base, char *buf, long off)
{
  uae_u32 mode;
  /* FELLOW CHANGE: a_inode *aino = (a_inode *) xmalloc (sizeof (a_inode)); */
  a_inode *aino = (a_inode *)xcalloc(sizeof(a_inode), 1);
  /* FELLOW BUGFIX (START) */
  if (aino == nullptr) return nullptr;
  /* FELLOW BUGFIX (END) */

  mode = do_get_mem_long((uae_u32 *)(buf + 1));
  buf += 5;
  aino->aname = my_strdup(buf);
  buf += 257;
  aino->nname = build_nname(base->nname, buf);
  buf += 257;
  aino->comment = *buf != '\0' ? my_strdup(buf) : nullptr;
  fsdb_fill_file_attrs(aino);
  aino->amigaos_mode = mode;
  aino->has_dbentry = 1;
  aino->dirty = 0;
  aino->db_offset = off;
  return aino;
}

a_inode *fsdb_lookup_aino_aname(a_inode *base, const char *aname)
{
  /* FELLOW BUGFIX (START) */
  a_inode *result;
  /* FELLOW BUGFIX (END) */
  FILE *f = get_fsdb(base, "rb");
  long off = 0;

  if (f == nullptr) return nullptr;

  for (;;)
  {
    char buf[1 + 4 + 257 + 257 + 81];
    if (fread(buf, 1, sizeof buf, f) < sizeof buf) break;
    /* FELLOW CHANGE (START): if (buf[0] != 0 && same_aname (buf + 5, aname)) { */
    /* the compiler's behaviour in the line above is undefined, that may work, that may not */
    if (buf[0] == 0) continue;
    if (same_aname(buf + 5, aname))
    {
      /* FELLOW CHANGE (END) */
      fclose(f);
      /* FELLOW BUGFIX (START):  return aino_from_buf (base, buf, off); */
      /* try to access the file */
      result = aino_from_buf(base, buf, off);
      if (result && (access(result->nname, R_OK) != -1))
        return (result);
      else
        return (nullptr);
      /* FELLOW BUGFIX (END) */
    }
    off += sizeof buf;
  }
  fclose(f);
  return nullptr;
}

a_inode *fsdb_lookup_aino_nname(a_inode *base, const char *nname)
{
  /* FELLOW BUGFIX */
  a_inode *result;
  /* FELLOW BUGFIX */
  FILE *f = get_fsdb(base, "rb");
  long off = 0;

  if (f == nullptr) return nullptr;

  for (;;)
  {
    char buf[1 + 4 + 257 + 257 + 81];
    if (fread(buf, 1, sizeof buf, f) < sizeof buf) break;
    /* FELLOW CHANGE (START): if (buf[0] != 0 && strcmp (buf + 5 + 257, nname) == 0) { */
    if (buf[0] == 0) continue;
    if (strcmp(buf + 5 + 257, nname) == 0)
    {
      /* FELLOW CHANGE (END) */
      fclose(f);
      /* FELLOW CHANGE (START): return aino_from_buf (base, buf, off); */
      result = aino_from_buf(base, buf, off);
      if (result && (access(result->nname, R_OK) != -1))
        return (result);
      else
        return (nullptr);
      /* FELLOW CHANGE (END) */
    }
  }
  fclose(f);
  return nullptr;
}

int fsdb_used_as_nname(a_inode *base, const char *nname)
{
  FILE *f = get_fsdb(base, "rb");
  if (f == nullptr) return 0;
  for (;;)
  {
    char buf[1 + 4 + 257 + 257 + 81];
    if (fread(buf, 1, sizeof buf, f) < sizeof buf) break;
    if (buf[0] == 0) continue;
    if (strcmp(buf + 5 + 257, nname) == 0)
    {
      fclose(f);
      /* FELLOW CHANGE (START): return 1; */
      if (access(nname, R_OK) != -1)
        return 1;
      else
        return 0;
      /* FELLOW CHANGE (END) */
    }
  }
  fclose(f);
  return 0;
}

static int needs_dbentry(a_inode *aino)
{
  /* FELLOW CHANGE: const char *an_begin, *nn_begin; */
  const char *nn_begin; /* an_begin was not referenced */

  if (aino->deleted) return 0;

  if (!fsdb_mode_representable_p(aino) || aino->comment != nullptr) return 1;

  nn_begin = nname_begin(aino->nname);
  return strcmp(nn_begin, aino->aname) != 0;
}

static void write_aino(FILE *f, a_inode *aino)
{
  char buf[1 + 4 + 257 + 257 + 81];
  buf[0] = aino->needs_dbentry;
  do_put_mem_long((uae_u32 *)(buf + 1), aino->amigaos_mode);
  strncpy(buf + 5, aino->aname, 256);
  buf[5 + 256] = '\0';
  strncpy(buf + 5 + 257, nname_begin(aino->nname), 256);
  buf[5 + 257 + 256] = '\0';
  strncpy(buf + 5 + 2 * 257, aino->comment ? aino->comment : "", 80);
  buf[5 + 2 * 257 + 80] = '\0';
  fwrite(buf, 1, sizeof buf, f);
  aino->has_dbentry = aino->needs_dbentry;
}

/* Write back the db file for a directory.  */

void fsdb_dir_writeback(a_inode *dir)
{
  FILE *f;
  int changes_needed = 0;
  int entries_needed = 0;
  a_inode *aino;

  /* First pass: clear dirty bits where unnecessary, and see if any work
   * needs to be done.  */
  for (aino = dir->child; aino; aino = aino->sibling)
  {
    int old_needs_dbentry = aino->needs_dbentry;
    aino->needs_dbentry = old_needs_dbentry;
    if (!aino->dirty) continue;
    aino->needs_dbentry = needs_dbentry(aino);
    entries_needed |= aino->needs_dbentry;
    if (!aino->needs_dbentry && !old_needs_dbentry)
      aino->dirty = 0;
    else
      changes_needed = 1;
  }
  if (!entries_needed)
  {
    kill_fsdb(dir);
    return;
  }

  if (!changes_needed) return;

  f = get_fsdb(dir, "r+b");
  if (f == nullptr)
  {
    f = get_fsdb(dir, "w+b");
    if (f == nullptr) /* This shouldn't happen... */
      return;
  }

  for (aino = dir->child; aino; aino = aino->sibling)
  {
    /* FELLOW REMOVE (unreferenced): off_t pos; */

    if (!aino->dirty) continue;
    aino->dirty = 0;

    if (!aino->has_dbentry)
    {
      aino->db_offset = fseek(f, 0, SEEK_END);
      aino->has_dbentry = 1;
    }
    else
      fseek(f, aino->db_offset, SEEK_SET);

    write_aino(f, aino);
  }
  fclose(f);
}
