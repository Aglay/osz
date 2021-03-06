/*
 * core/service.c
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
 * @brief System service loader and ELF parser
 */

#include <arch.h>

/* external resources */
extern phy_t screen[2];
extern phy_t pdpe;
extern uint64_t *syslog_buf;
extern char *drvs;
extern char *drvs_end;

/* pids of services. Negative pids are service ids and looked up in this table */
dataseg pid_t  services[NUMSRV];

/* needs screen pointer mapping */
dataseg uint8_t scrptr = 0;

/* location of fstab in mapped initrd */
dataseg uint64_t fstab;
dataseg uint64_t fstab_size;

/* pointer to mapped buffer */
dataseg virt_t buffer_ptr;

/**
 * Initialize a non-special system service
 */
void service_init(int subsystem, char *fn)
{
    char *s, *f;
    char so[256];
    char *cmd = fn;
    while(cmd[0]!='/')
        cmd++;
    cmd++;
    int l=kstrlen(cmd);
    tcb_t *tcb = task_new(cmd, PRI_SRV);
    kmemcpy(&tcb->owner, "core", 5);
    // map executable
    if(elf_load(fn) == (void*)(-1)) {
        syslog_early("WARNING unable to load ELF from %s", fn);
        return;
    }
    // little trick to support syslog buffer
    if(subsystem==SRV_syslog)
        buffer_ptr=task_mapbuf(syslog_buf,nrlogmax);
    // map libc
    elf_loadso("sys/lib/libc.so");
    // load modules for system services
    kmemcpy(&so[0], "sys/drv/", 8);
    for(s=drvs;s<drvs_end;) {
        f = s; while(s<drvs_end && *s!=0 && *s!='\n') s++;
        if(f[0]=='*' && f[1]==9 && f+2+l<drvs_end && f[2+l]=='/' && !kmemcmp(f+2,cmd,l)) {
            f+=2;
            if(s-f<255-8) {
                kmemcpy(&so[8], f, s-f);
                so[s-f+8]=0;
                elf_loadso(so);
            }
            continue;
        }
        // failsafe
        if(s>=drvs_end || *s==0) break;
        if(*s=='\n') s++;
    }
    // map libc and other libraries
    elf_neededso(1);

    // dynamic linker
    if(elf_rtlink()) {
        // map the first page in bss
        vmm_mapbss(tcb, BSS_ADDRESS, (phy_t)pmm_alloc(1), __PAGESIZE, PG_USER_RW);
        tcb->allocmem++;

        // add to queue so that scheduler will know about this task
        sched_add(tcb);

        services[-subsystem] = tcb->pid;
        syslog_early(" %s -%d pid %x",cmd,-subsystem,tcb->pid);
    } else {
        syslog_early("WARNING task check failed for /%s", fn);
    }
}

/*** critical tasks, need special initialization ***/

/**
 * Initialize the file system service, the "FS" task
 */
void fs_init()
{
    char *s, *f;
    char fn[256];
    fstab = (uint64_t)fs_locate("sys/etc/fstab");
    fstab_size=fs_size;
    tcb_t *tcb = task_new("FS", PRI_SRV);
    kmemcpy(&tcb->owner, "core", 5);
    // map file system dispatcher
    if(elf_load("sys/fs") == (void*)(-1)) {
        kpanic("unable to load ELF from /sys/fs");
    }
    // map libc
    elf_loadso("sys/lib/libc.so");

    // load file system drivers
    if(drvs==NULL) {
        // hardcoded if driver list not found
        // should not happen!
        elf_loadso("sys/drv/fs/gpt.so");    // disk
        elf_loadso("sys/drv/fs/fsz.so");    // initrd and OS/Z partitions
        elf_loadso("sys/drv/fs/vfat.so");   // EFI boot partition
    } else {
        kmemcpy(&fn[0], "sys/drv/", 8);
        for(s=drvs;s<drvs_end;) {
            f = s; while(s<drvs_end && *s!=0 && *s!='\n') s++;
            if(f[0]=='*' && f[1]==9 && f[2]=='f' && f[3]=='s') {
                f+=2;
                if(s-f<255-8) {
                    kmemcpy(&fn[8], f, s-f);
                    fn[s-f+8]=0;
                    elf_loadso(fn);
                }
                continue;
            }
            // failsafe
            if(s>=drvs_end || *s==0) break;
            if(*s=='\n') s++;
        }
    }
    elf_neededso(1);

    // dynamic linker
    if(elf_rtlink()) {
        // map the first page in bss
        vmm_mapbss(tcb, BSS_ADDRESS, (phy_t)pmm_alloc(1), __PAGESIZE, PG_USER_RW);
        tcb->allocmem++;

        // map initrd in "fs" task's memory
        vmm_mapbss(tcb,BUF_ADDRESS,bootboot.initrd_ptr, bootboot.initrd_size, PG_USER_RW);

        // add to queue so that scheduler will know about this task
        sched_add(tcb);

        services[-SRV_FS] = tcb->pid;
        syslog_early(" %s -%d pid %x","FS",-SRV_FS,tcb->pid);
    } else {
        kpanic("task check failed for /sys/fs");
    }
}

/**
 * Initialize the user interface service, the "UI" task
 */
void ui_init()
{
    int i;
    tcb_t *tcb = task_new("UI", PRI_SRV);
    kmemcpy(&tcb->owner, "core", 5);

    // map user interface code
    if(elf_load("sys/ui") == (void*)(-1)) {
        kpanic("unable to load ELF from /sys/ui");
    }
    // map window decoratorator
//    elf_loadso("lib/ui/decor.so");
    // map libc and other libraries
    elf_neededso(0);

    // dynamic linker
    if(elf_rtlink()) {
        // add to queue so that scheduler will know about this task
        sched_add(tcb);

        services[-SRV_UI] = tcb->pid;
        syslog_early(" %s -%d pid %x","UI",-SRV_UI,tcb->pid);

        // allocate and map screen buffer A
        virt_t bss=BUF_ADDRESS;
        for(i = ((bootboot.fb_width * bootboot.fb_height * 4 +
            __SLOTSIZE - 1) / __SLOTSIZE) * (display>=DSP_STEREO_MONO?2:1);i>0;i--) {
            vmm_mapbss(tcb,bss, (phy_t)pmm_allocslot(), __SLOTSIZE, PG_USER_RW);
            // save it for SYS_swapbuf
            if(!screen[0]) {
                screen[0]=pdpe;
            }
            bss += __SLOTSIZE;
        }
    } else {
        kpanic("task check failed for /sys/ui");
    }
}

/**
 * Initialize a device driver task
 */
void drv_init(char *driver)
{
    int i = kstrlen(driver);
    // get driver class and name from path
    char *drvname=driver + i;
    while(drvname>driver && *(drvname-1)!='/') drvname--;
    if(drvname>driver) {
        drvname--;
        while(drvname>driver && *(drvname-1)!='/') drvname--;
    }
    while(i>0 && driver[i]!='.') i--;
    driver[i]=0;

    // create a new task...
    tcb_t *tcb = task_new(drvname, PRI_DRV);
    kmemcpy(&tcb->owner, "core", 5);
    driver[i]='.';
    // ...start with driver event dispatcher
    if(elf_load("sys/driver") == (void*)(-1)) {
        kpanic("unable to load ELF from /sys/driver");
    }
    // map the environment so that drivers can parse it for their config
    buffer_ptr=task_mapbuf(environment,1);
    // map libc
    elf_loadso("sys/lib/libc.so");
    // map the real driver as a shared object
    elf_loadso(driver);
    elf_neededso(1);

    // dynamic linker
    scrptr = false;
    if(elf_rtlink()) {
        // add to queue so that scheduler will know about this task
        sched_add(tcb);

        driver[i]=0;
        syslog_early(" %s %x pid %x",drvname,tcb->memroot,tcb->pid);
        driver[i]='.';

        //do we need to map screen and framebuffer?
        if(scrptr) {
            // allocate and map screen buffer B
            virt_t bss=BUF_ADDRESS;
            for(i = ((bootboot.fb_width * bootboot.fb_height * 4 +
                __SLOTSIZE - 1) / __SLOTSIZE) * (display>=DSP_STEREO_MONO?2:1);i>0;i--) {
                vmm_mapbss(tcb, bss, (phy_t)pmm_allocslot(), __SLOTSIZE, PG_USER_DRVMEM);
                // save it for SYS_swapbuf
                if(!screen[1]) {
                    screen[1]=pdpe;
                }
                bss += __SLOTSIZE;
            }
            // map framebuffer in next PDE
            bss=(virt_t)BUF_ADDRESS + ((virt_t)__SLOTSIZE * ((virt_t)__PAGESIZE / 8));
            phy_t fbp=(phy_t)bootboot.fb_ptr;
            for(i = (bootboot.fb_scanline * bootboot.fb_height * 4 + __SLOTSIZE - 1) / __SLOTSIZE;i>0;i--) {
                vmm_mapbss(tcb,bss,fbp,__SLOTSIZE, PG_USER_DRVMEM);
                bss += __SLOTSIZE;
                fbp += __SLOTSIZE;
            }
            services[-SRV_video] = tcb->pid;
        }
    } else {
        syslog_early("WARNING task check failed for /%s", driver);
    }
}
