 /*
  * UAE - The Un*x Amiga Emulator
  * 
  * Generic thread support doesn't exist.
  * 
  * Copyright 1997 Bernd Schmidt
  */

/* FELLOW IN (START)-----------------

  This file has been adapted for use in WinFellow.
  It originates from the UAE 0.8.22 source code distribution.
  
  The original file is no longer called penguin.h but has been renamed to thread.h.
  This has not been changed here to not confuse the CVS repository.

  Torsten Enderling (carfesh@gmx.net) 2004

  @(#) $Id: PENGUIN.H,v 1.1.1.1.2.2 2004-05-27 09:41:12 carfesh Exp $

   FELLOW IN (END)------------------- */

#undef SUPPORT_PENGUINS
#undef SUPPORT_THREADS

typedef int uae_sem_t;
#define uae_sem_init(a,b,c)
#define uae_sem_destroy(a)
#define uae_sem_post(a)
#define uae_sem_wait(a)
#define uae_sem_trywait(a) 0
#define uae_sem_getvalue(a,b) 0

typedef int smp_comm_pipe;
#define write_comm_pipe_int(a,b,c)
#define read_comm_pipe_int_blocking(a) 0
#define write_comm_pipe_pvoid(a,b,c)
#define read_comm_pipe_pvoid_blocking(a) 0
#define init_comm_pipe(a,b,c)
#define comm_pipe_has_data(a) 0

typedef int uae_thread_id;
