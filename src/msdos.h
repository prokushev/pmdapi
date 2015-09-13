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

#ifndef __MSDOS_H__
#define __MSDOS_H__

#define MSDOS_MAX_MEM_ALLOCS 1024

struct msdos_struct {
  int is_32;
  struct pmaddr_s mouseCallBack, PS2mouseCallBack; /* user\'s mouse routine */
  far_t XMS_call;
  /* used when passing a DTA higher than 1MB */
  unsigned short user_dta_sel;
  unsigned long user_dta_off;
  unsigned short user_psp_sel;
  unsigned short lowmem_seg;
  dpmi_pm_block mem_map[MSDOS_MAX_MEM_ALLOCS];
  far_t rmcb;
};

extern void msdos_setup(void);
extern void msdos_reset(u_short emm_s);
extern void msdos_init(int is_32, unsigned short mseg);
extern void msdos_done(void);
extern int msdos_get_lowmem_size(void);
extern int msdos_pre_extender(struct sigcontext *scp, int intr,
	struct RealModeCallStructure *rmreg, int *r_mask);
extern int msdos_post_extender(struct sigcontext *scp, int intr,
	const struct RealModeCallStructure *rmreg);
extern int msdos_fault(struct sigcontext *scp);

enum { MSDOS_NONE, MSDOS_RM, MSDOS_DONE };

#endif
