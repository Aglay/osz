/*
 * core/x86_64/sys.c
 *
 * Copyright 2016 CC-by-nc-sa bztsrc@github
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * You are free to:
 *
 * - Share — copy and redistribute the material in any medium or format
 * - Adapt — remix, transform, and build upon the material
 *     The licensor cannot revoke these freedoms as long as you follow
 *     the license terms.
 *
 * Under the following terms:
 *
 * - Attribution — You must give appropriate credit, provide a link to
 *     the license, and indicate if changes were made. You may do so in
 *     any reasonable manner, but not in any way that suggests the
 *     licensor endorses you or your use.
 * - NonCommercial — You may not use the material for commercial purposes.
 * - ShareAlike — If you remix, transform, or build upon the material,
 *     you must distribute your contributions under the same license as
 *     the original.
 *
 * @brief System routines
 */

#include "platform.h"
#include "../../../loader/bootboot.h"
#include "../env.h"
#include "acpi.h"
#include "pci.h"

/* external resources */
extern ccb_t ccb;                   // CPU Control Block
extern uint32_t fg;
extern char rebootprefix[];
extern char poweroffprefix[];
extern char poweroffsuffix[];
extern uint64_t pt;
extern char *syslog_buf;

extern void kprintf_center(int w, int h);
extern void acpi_init();
extern void acpi_poweroff();
extern void pci_init();
extern void idle();
extern tcb_t *sched_get_tcb(pid_t task);
#if DEBUG
extern uint8_t dbg_enabled;
#endif

/* device drivers map */
dataseg char *drvs;
dataseg char *drvs_end;
dataseg phy_t screen[2];
dataseg char fn[256];
dataseg uint8_t sys_fault;
dataseg uint8_t idle_first;

/* system tables */
dataseg uint64_t systables[8];

/**
 *  reboot computer
 */
void sys_reset()
{
    // say we're rebooting (on serial too)
    kprintf_init();
#if DEBUG
    dbg_putchar(13);
    dbg_putchar(10);
#endif
    kprintf(rebootprefix);
    // reboot computer
    __asm__ __volatile__ ("movb $0xFE, %al; outb %al, $0x64");
    // if it didn't work, show a message and freeze. Should never happen.
    fg = 0x29283f;
    kprintf_center(20, -8);
    kprintf(poweroffsuffix);
#if DEBUG
    dbg_putchar(13);
    dbg_putchar(10);
#endif
    __asm__ __volatile__ ("1: cli; hlt; jmp 1b");
}

/**
 * turn off computer
 */
void sys_disable()
{
    // say we're finished (on serial too)
    kprintf_init();
#if DEBUG
    dbg_putchar(13);
    dbg_putchar(10);
#endif
    kprintf(poweroffprefix);
    // Poweroff real hardware
    acpi_poweroff();
    // Bochs poweroff hack
    char *s = "Shutdown";
    while (*s) {
        __asm__ __volatile__ ("movw $0x8900, %%dx; outb %0, %%dx" : : "a"(*s));
        s++;
    }
    // if it didn't work, show a message and freeze.
    fg = 0x29283f;
    kprintf_center(20, -8);
    kprintf(poweroffsuffix);
#if DEBUG
    dbg_putchar(13);
    dbg_putchar(10);
#endif
    __asm__ __volatile__ ("1: cli; hlt; jmp 1b");
}

/**
 * switch to the first user task and start executing it
 */
__inline__ void sys_enable()
{
    tcb_t *tcb = (tcb_t*)0;
    tcb_t *fstcb = (tcb_t*)(&tmpmap);

#if DEBUG
    // enable debugger, it can be used only with task mappings
    dbg_enabled = true;
#endif
    sys_fault = false;

    // map FS task's TCB
    kmap((uint64_t)&tmpmap,
        (uint64_t)(services[-SRV_FS]*__PAGESIZE),
        PG_CORE_NOCACHE);

    syslog_early("Initializing");
    // fake an interrupt handler return to force first task switch
    __asm__ __volatile__ (
        // switch to first task's address space
        "mov %0, %%rax; mov %%rax, %%cr3;"
        // clear ABI arguments
        "xorq %%rdi, %%rdi;xorq %%rsi, %%rsi;xorq %%rdx, %%rdx;xorq %%rcx, %%rcx;"
        // "return" to the task
        "movq %1, %%rsp; movq %2, %%rbp; xchg %%bx, %%bx; iretq" :
        :
        "a"(fstcb->memroot), "b"(&tcb->rip), "i"(TEXT_ADDRESS));
}

/**
 * Initialize the "idle" task and device drivers
 */
void sys_init()
{
    // load driver database
    char *c, *f;
    drvs = (char *)fs_locate("sys/drivers");
    drvs_end = drvs + fs_size;

    /*** Platform specific initialization ***/
    /* create idle task */
    tcb_t *tcb = task_new("idle", PRI_IDLE);
    // modify TCB for idle task. Don't add to scheduler queue, normally it will never be scheduled
    //start executing a special function.
    tcb->rip = (uint64_t)&idle;
    tcb->cs = 0x8;  //ring 0 selectors
    tcb->ss = 0x10;
    idle_mapping = tcb->memroot;
    idle_first = true;

    syslog_early("Device drivers");
    /* interrupt service routines (idt, pic, ioapic etc.) */
    isr_init();

    /* detect devices and load drivers for them */
    if(drvs==NULL) {
        // should never happen!
#if DEBUG
        kprintf("WARNING missing /sys/drivers\n");
#endif
        syslog_early("WARNING missing /sys/drivers\n");
        // hardcoded legacy devices if driver list not found
        drv_init("sys/drv/input/ps2.so");
        drv_init("sys/drv/display/fb.so");
    } else {
        kmemcpy(&fn[0], "sys/drv/", 8);
        // load devices which are not coverable by bus enumeration
        for(c=drvs;c<drvs_end;) {
            f = c; while(c<drvs_end && *c!=0 && *c!='\n') c++;
            // skip filesystem drivers
            if(f[0]=='*' && f[1]==9 && (f[2]!='f' || f[3]!='s')) {
                f+=2;
                if(c-f<255-8) {
                    kmemcpy(&fn[8], f, c-f);
                    fn[c-f+8]=0;
                    drv_init(fn);
                }
                continue;
            }
            // failsafe
            if(c>=drvs_end || *c==0) break;
            if(*c=='\n') c++;
        }
/*
        // parse ACPI
        acpi_init();
        // enumerate PCI BUS
        pci_init();
        // ...enumarate other system buses
*/
    }
    syslog_early("Services");
}

/*** Called when the "idle" task first scheduled ***/
void sys_ready()
{
    tcb_t *tcb=sched_get_tcb(services[-SRV_init]);

    /* reset early kernel console */
    kprintf_reset();

    /* finish up ISR initialization */
    isr_fini();
    /* log we're ready */
    syslog_early("Ready. Memory %d of %d pages free.", pmm.freepages, pmm.totalpages);

    /* Display ready message */
    kprintf("OS/Z ready. Allocated %d pages out of %d",
        pmm.totalpages - pmm.freepages, pmm.totalpages);
    kprintf(", free %d.%d%%\n",
        pmm.freepages*100/(pmm.totalpages+1), (pmm.freepages*1000/(pmm.totalpages+1))%10);
    // breakpoint for bochs
    __asm__ __volatile__("xchg %bx,%bx");

    if(debug&DBG_LOG)
        kprintf(syslog_buf);

    /* disable scroll pause */
    scry = -1;

    /* mount filesystems. Make it look like init had sent the message so FS will notify init when it's done */
    sched_awake(tcb);
    task_map(tcb->memroot);
    msg_sends(EVT_DEST(SRV_FS) | EVT_FUNC(SYS_mountfs),0,0,0,0,0,0);
}
