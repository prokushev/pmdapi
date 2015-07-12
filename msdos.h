/* 
 * (C) Copyright 1992, ..., 2004 the "DOSEMU-Development-Team".
 *
 * for details see file COPYING in the DOSEMU distribution
 */

/* 	MS-DOS API translator for DOSEMU\'s DPMI Server
 *
 * DANG_BEGIN_MODULE msdos.h
 *
 * REMARK
 * MS-DOS API translator allows DPMI programs to call DOS service directly
 * in protected mode.
 *
 * /REMARK
 * DANG_END_MODULE
 *
 * First Attempted by Dong Liu,  dliu@rice.njit.edu
 *
 */

extern int msdos_pre_extender(struct sigcontext *scp, int intr);
extern int msdos_post_extender(struct sigcontext *scp, int intr);
extern int msdos_fault(struct sigcontext *scp);
extern void msdos_pre_exec(struct sigcontext *scp);
extern void msdos_post_exec(void);

#define MSDOS_DONE 1
#define MSDOS_ALT_RET 2
#define MSDOS_ALT_ENT 4
#define MSDOS_NEED_FORK 8
