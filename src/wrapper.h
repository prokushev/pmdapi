/*
 * (C) Copyright 1992, ..., 2004 the "DOSEMU-Development-Team".
 *
 * for details see file COPYING in the DOSEMU distribution
 */

/* this is for the DPMI support */
#ifndef DPMI_H
#define DPMI_H

#include <dpmi.h>
#include <stdint.h>
#include <stddef.h>
#include "sigcontext.h"
#include "cpu.h"

typedef unsigned short us;
typedef uint32_t dosaddr_t;

#define sigcontext_struct sigcontext

#define DPMI_MAX_CLIENTS	32	/* maximal number of clients */
#define PAGE_MASK	(~(PAGE_SIZE-1))
/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

enum { es_INDEX, cs_INDEX, ss_INDEX, ds_INDEX, fs_INDEX, gs_INDEX,
  eax_INDEX, ebx_INDEX, ecx_INDEX, edx_INDEX, esi_INDEX, edi_INDEX,
  ebp_INDEX, esp_INDEX, eip_INDEX, eflags_INDEX };

typedef __dpmi_paddr DPMI_INTDESC;
struct pmaddr_s
{
    unsigned int	offset;
    unsigned short	selector;
};
typedef struct dpmi_pm_block_stuct {
  struct   dpmi_pm_block_stuct *next;
  unsigned int handle;
  unsigned int size;
  dosaddr_t base;
  u_short  *attrs;
  int linear;
} dpmi_pm_block;

struct RealModeCallStructure {
  unsigned int edi;
  unsigned int esi;
  unsigned int ebp;
  unsigned int esp_reserved;
  unsigned int ebx;
  unsigned int edx;
  unsigned int ecx;
  unsigned int eax;
  unsigned short flags;
  unsigned short es;
  unsigned short ds;
  unsigned short fs;
  unsigned short gs;
  unsigned short ip;
  unsigned short cs;
  unsigned short sp;
  unsigned short ss;
} __attribute__((packed));

int ValidAndUsedSelector(unsigned short selector);

extern int ConvertSegmentToDescriptor(unsigned short segment);
extern int ConvertSegmentToCodeDescriptor(unsigned short segment);

dpmi_pm_block DPMImalloc(unsigned long size);
int DPMIfree(unsigned long handle);
dpmi_pm_block DPMIrealloc(unsigned long handle, unsigned long size);

extern DPMI_INTDESC dpmi_get_interrupt_vector(unsigned char num);
extern void dpmi_set_interrupt_vector(unsigned char num, DPMI_INTDESC desc);
void GetFreeMemoryInformation(unsigned int *lp);
int GetDescriptor(us selector, unsigned long *lp);

extern int SetSegmentBaseAddress(unsigned short selector,
					unsigned long baseaddr);
unsigned long GetSegmentBaseAddress(unsigned short);
unsigned long GetSegmentLimit(unsigned short);
extern unsigned int GetSegmentBase(unsigned short);
int dpmi_mhp_get_selector_size(int sel);
extern int SetSegmentLimit(unsigned short, unsigned int);
extern unsigned short AllocateDescriptors(int);
extern int FreeDescriptor(unsigned short selector);
extern void FreeSegRegs(struct sigcontext *scp, unsigned short selector);
extern far_t allocate_realmode_callback(void (*handler)(
	struct RealModeCallStructure *));
extern int DPMI_free_realmode_callback(u_short seg, u_short off);

extern void copy_context(struct sigcontext_struct *d,
    struct sigcontext_struct *s, int copy_fpu);
extern void save_pm_regs(struct sigcontext_struct *);
extern void restore_pm_regs(struct sigcontext_struct *);

void *SEL_ADR(unsigned short sel, unsigned int reg);
void *SEL_ADR_CLNT(unsigned short sel, unsigned int reg, int is_32);
u_short DPMI_ldt_alias(void);

extern void pm_to_rm_regs(struct sigcontext_struct *scp, unsigned int mask);
extern void rm_to_pm_regs(struct sigcontext_struct *scp, unsigned int mask);

extern unsigned short dpmi_sel(void);
extern unsigned short dpmi_data_sel(void);
void fake_call_to(int cs, int ip);

#define pushw(base, ptr, val) \
	do { \
		ptr = (Bit16u)(ptr - 1); \
		WRITE_BYTE((base) + ptr, (val) >> 8); \
		ptr = (Bit16u)(ptr - 1); \
		WRITE_BYTE((base) + ptr, val); \
	} while(0)

#define popb(base, ptr) \
	({ \
		Bit8u __res = READ_BYTE((base) + ptr); \
		ptr = (Bit16u)(ptr + 1); \
		__res; \
	})

#define popw(base, ptr) \
	({ \
		Bit8u __res0, __res1; \
		__res0 = READ_BYTE((base) + ptr); \
		ptr = (Bit16u)(ptr + 1); \
		__res1 = READ_BYTE((base) + ptr); \
		ptr = (Bit16u)(ptr + 1); \
		(__res1 << 8) | __res0; \
	})

#define DPMI_SEG 0
#define DPMI_OFF 0
#define DPMI_ADD 0
#define HLT_OFF(x) 0
#define EMM_SEGMENT 0
#define DTA_Para_ADD 0
#define DOS_LONG_READ_SEG 0
#define DOS_LONG_READ_OFF 0
#define DOS_LONG_WRITE_SEG 0
#define DOS_LONG_WRITE_OFF 0
#define MSDOS_XMS_call 0
#define MSDOS_API_call 0
#define MSDOS_rmcb_call 0
#define DPMI_sel_code_start 0
#define DPMI_sel_data_start 0
#define DPMI_SEL_OFF(x) (x-DPMI_sel_code_start)
#define DPMI_DATA_OFF(x) (x-DPMI_sel_data_start)
#define _CS 0
#define _IP 0
#define MSDOS_return_from_pm 0

#define D_printf(...)
#define g_printf(...)
#define error(...)
#define dosemu_error(...)
#define snprintf(a,b,c,d) sprintf(a,c,d)
#define ConvertSegmentToDescriptor_lim(a,b) ConvertSegmentToDescriptor(a)
#define MEMCPY_2DOS(dos_addr, unix_addr, n) \
	memcpy(LINEAR2UNIX(dos_addr), (unix_addr), (n))
#define MEMSET_DOS(dos_addr, val, n) \
        memset(LINEAR2UNIX(dos_addr), (val), (n))
#define MEMCPY_2UNIX(unix_addr, dos_addr, n) \
	memcpy((unix_addr), LINEAR2UNIX(dos_addr), (n))
static inline void *LINEAR2UNIX(unsigned int addr)
{
	return (void*)addr;
}

#define min(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	_x < _y ? _x : _y; })

extern unsigned char *mem_base;
#define MK_FP32(s,o)		((void *)&mem_base[SEGOFF2LINEAR(s,o)])
#define LINP(a) ((unsigned char *)0 + (a))
static inline unsigned char *MEM_BASE32(dosaddr_t a)
{
    uint32_t off = (uint32_t)(ptrdiff_t)(mem_base + a);
    return LINP(off);
}
static inline dosaddr_t DOSADDR_REL(const unsigned char *a)
{
    return (a - mem_base);
}

u_short dos_get_psp(void);
void lrhlp_setup(far_t rmcb, int is_w);

#endif /* DPMI_H */
