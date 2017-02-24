/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Purpose: glue between msdos.c and the rest of dosemu
 * This is needed to keep msdos.c portable to djgpp.
 *
 * Author: Stas Sergeev
 *
 * Currently there are only helper stubs here.
 * The helpers itself are in bios.S.
 * TODO: port bios.S asm helpers to C and put here
 */

//#include "emu.h"
#include "cpu.h"
//#include "utilities.h"
//#include "int.h"
//#include "hlt.h"
//#include "coopth.h"
#include "dpmi.h"
//#include "dpmisel.h"
#include "calls.h"
#include "entry.h"
#include "msdoshlp.h"
#include <assert.h>
#include <sys/segments.h>

#define MAX_CBKS 3
struct msdos_ops {
    void (*api_call)(struct sigcontext *scp, void *arg);
    void *api_arg;
    void (*api_winos2_call)(struct sigcontext *scp, void *arg);
    void *api_winos2_arg;
    void (*xms_call)(struct RealModeCallStructure *rmreg, void *arg);
    void *xms_arg;
    void (*rmcb_handler[MAX_CBKS])(struct sigcontext *scp,
	const struct RealModeCallStructure *rmreg, int is_32, void *arg);
    void *rmcb_arg[MAX_CBKS];
    void (*rmcb_ret_handler[MAX_CBKS])(struct sigcontext *scp,
	struct RealModeCallStructure *rmreg, int is_32);
    u_short cb_es;
    u_int cb_edi;
};
static struct msdos_ops msdos;

struct exec_helper_s {
    int initialized;
    int tid;
    far_t entry;
    far_t s_r;
    u_char len;
};
static struct exec_helper_s exec_helper;

static void lrhlp_setup(far_t rmcb)
{
#if 0
#define MK_LR_OFS(ofs) ((long)(ofs)-(long)MSDOS_lr_start)
    WRITE_WORD(SEGOFF2LINEAR(DOS_LONG_READ_SEG, DOS_LONG_READ_OFF +
		     MK_LR_OFS(MSDOS_lr_entry_ip)), rmcb.offset);
    WRITE_WORD(SEGOFF2LINEAR(DOS_LONG_READ_SEG, DOS_LONG_READ_OFF +
		     MK_LR_OFS(MSDOS_lr_entry_cs)), rmcb.segment);
#endif
}

static void lwhlp_setup(far_t rmcb)
{
#if 0
#define MK_LW_OFS(ofs) ((long)(ofs)-(long)MSDOS_lw_start)
    WRITE_WORD(SEGOFF2LINEAR
	       (DOS_LONG_WRITE_SEG,
		DOS_LONG_WRITE_OFF + MK_LW_OFS(MSDOS_lw_entry_ip)),
	       rmcb.offset);
    WRITE_WORD(SEGOFF2LINEAR
	       (DOS_LONG_WRITE_SEG,
		DOS_LONG_WRITE_OFF + MK_LW_OFS(MSDOS_lw_entry_cs)),
	       rmcb.segment);
#endif
}

#if 0
static void s_r_call(u_char al, u_short es, u_short di)
{
    u_short saved_ax = LWORD(eax), saved_es = SREG(es), saved_di = LWORD(edi);

    LO(ax) = al;
    SREG(es) = es;
    LWORD(edi) = di;
    do_call_back(exec_helper.s_r.segment, exec_helper.s_r.offset);
    LWORD(eax) = saved_ax;
    SREG(es) = saved_es;
    LWORD(edi) = saved_di;
}

static void exechlp_thr(void *arg)
{
    uint32_t saved_flags;

    assert(LWORD(esp) >= exec_helper.len);
    LWORD(esp) -= exec_helper.len;
    s_r_call(0, SREG(ss), LWORD(esp));
    do_int_call_back(0x21);
    saved_flags = REG(eflags);
    s_r_call(1, SREG(ss), LWORD(esp));
    REG(eflags) = saved_flags;
    LWORD(esp) += exec_helper.len;
}

static void exechlp_hlt(Bit16u off, void *arg)
{
    fake_iret();
    coopth_start(exec_helper.tid, exechlp_thr, NULL);
}
#endif

static void exechlp_setup(void)
{
    struct pmaddr_s pma;
    exec_helper.len = DPMI_get_save_restore_address(&exec_helper.s_r, &pma);
    if (!exec_helper.initialized) {
#if 0
	emu_hlt_t hlt_hdlr = HLT_INITIALIZER;
	hlt_hdlr.name = "msdos exec";
	hlt_hdlr.func = exechlp_hlt;
	exec_helper.entry.offset = hlt_register_handler(hlt_hdlr);
	exec_helper.entry.segment = BIOS_HLT_BLK_SEG;
	exec_helper.tid = coopth_create("msdos exec thr");
#endif
	exec_helper.initialized = 1;
    }
}

#define dpmi_sel() _my_cs()
#define DPMI_SEL_OFF(x) (uintptr_t)entry_##x

static void handler(struct sigcontext *scp, int n)
{
    struct RealModeCallStructure *rmreg;
    unsigned long base;
    __dpmi_get_segment_base_address(_es, &base);
    rmreg = (struct RealModeCallStructure *)(base + _edi);
    msdos.rmcb_handler[n](scp, rmreg, clnt_is_32, msdos.rmcb_arg[n]);
    do_pm_call(scp);
    msdos.rmcb_ret_handler[n](scp, rmreg, clnt_is_32);
}

#define HNDL(n) \
void MSDOS_rmcb_call##n(struct sigcontext *scp) \
{ \
    handler(scp, n); \
}
HNDL(0)
HNDL(1)
HNDL(2)

static int get_cb(int num)
{
    switch (num) {
    case 0:
	return DPMI_SEL_OFF(MSDOS_rmcb_call0);
    case 1:
	return DPMI_SEL_OFF(MSDOS_rmcb_call1);
    case 2:
	return DPMI_SEL_OFF(MSDOS_rmcb_call2);
    }
    return 0;
}

struct pmaddr_s get_pmcb_handler(void (*handler)(struct sigcontext *,
	const struct RealModeCallStructure *, int, void *),
	void *arg,
	void (*ret_handler)(struct sigcontext *,
	struct RealModeCallStructure *, int),
	int num)
{
    struct pmaddr_s ret;
    assert(num < MAX_CBKS);
    msdos.rmcb_handler[num] = handler;
    msdos.rmcb_arg[num] = arg;
    msdos.rmcb_ret_handler[num] = ret_handler;
    ret.selector = dpmi_sel();
    ret.offset = get_cb(num);
    return ret;
}

void MSDOS_API_call(struct sigcontext *scp)
{
    msdos.api_call(scp, msdos.api_arg);
}

void MSDOS_API_WINOS2_call(struct sigcontext *scp)
{
    msdos.api_winos2_call(scp, msdos.api_winos2_arg);
}

struct pmaddr_s get_pm_handler(enum MsdOpIds id,
	void (*handler)(struct sigcontext *, void *), void *arg)
{
    struct pmaddr_s ret;
    switch (id) {
    case API_CALL:
	msdos.api_call = handler;
	msdos.api_arg = arg;
	ret.selector = dpmi_sel();
	ret.offset = DPMI_SEL_OFF(MSDOS_API_call);
	break;
    case API_WINOS2_CALL:
	msdos.api_winos2_call = handler;
	msdos.api_winos2_arg = arg;
	ret.selector = dpmi_sel();
	ret.offset = DPMI_SEL_OFF(MSDOS_API_WINOS2_call);
	break;
    default:
	dosemu_error("unknown pm handler\n");
	ret = (struct pmaddr_s){ 0, 0 };
	break;
    }
    return ret;
}

void MSDOS_XMS_call(struct sigcontext *scp)
{
    struct RealModeCallStructure rmreg;
    msdos.xms_call(&rmreg, msdos.api_arg);
    /* FIXME! do rm call */
}

struct pmaddr_s get_pmrm_handler(enum MsdOpIds id, void (*handler)(
	struct RealModeCallStructure *, void *), void *arg)
{
    struct pmaddr_s ret;
    switch (id) {
    case XMS_CALL:
	msdos.xms_call = handler;
	msdos.xms_arg = arg;
	ret.selector = dpmi_sel();
	ret.offset = DPMI_SEL_OFF(MSDOS_XMS_call);
	break;
    default:
	dosemu_error("unknown pmrm handler\n");
	ret = (struct pmaddr_s){ 0, 0 };
	break;
    }
    return ret;
}

far_t get_lr_helper(far_t rmcb)
{
    lrhlp_setup(rmcb);
#if 0
    return (far_t){ .segment = DOS_LONG_READ_SEG,
	    .offset = DOS_LONG_READ_OFF };
#else
    return (far_t){ .segment = 0,
	    .offset = 0 };
#endif
}

far_t get_lw_helper(far_t rmcb)
{
    lwhlp_setup(rmcb);
#if 0
    return (far_t){ .segment = DOS_LONG_WRITE_SEG,
	    .offset = DOS_LONG_WRITE_OFF };
#else
    return (far_t){ .segment = 0,
	    .offset = 0 };
#endif
}

far_t get_exec_helper(void)
{
    exechlp_setup();
    return exec_helper.entry;
}
