#include "uae2fell.h"
#include "autoconf.h"

extern void expamem_init_filesys();
extern void expamem_map_filesys(uint32_t mapping);
extern void write_filesys_config (struct uaedev_mount_info *mountinfo,
			   const char *unexpanded, const char *default_path, FILE *f);
extern char uaehf0[256];
extern uint32_t hardfile_size;
extern struct uaedev_mount_info mountinfo;
