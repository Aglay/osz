/*
 * sys/osZ.h
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
 * @brief OS/Z specific system types
 */

#ifndef	_OSZ_TYPES_H
#define	_OSZ_TYPES_H	1

#define public __attribute__ ((__visibility__("default")))
#define private __attribute__ ((__visibility__("hidden")))

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef null
#define null NULL
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef true
#define true 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef false
#define false 0
#endif

#ifndef _AS
#include <stdint.h>

#define c_assert(c) extern char cassert[(c)?0:-1]

typedef uint64_t evt_t;

typedef struct {
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t Data4[8];
} __attribute__((packed)) uuid_t;
#define UUID_ACCESS(a) (a.Data4[7])

// type returned by syscalls clcall() and clrecv()
typedef struct {
    evt_t evt;
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t ts;
} __attribute__((packed)) msg_t;
// bits in evt: 63TTT..TTTPFFFFFFFFFFFFFFF0
//  where T is a thread id or subsystem id, P true if message has a pointer,
//  F is a function number from 1 to 32767. Function number 0 is reserved.
#define EVT_DEST(t) ((uint64_t)(t)<<16)
#define EVT_SENDER(m) ((pid_t)((m)>>16))
#define EVT_FUNC(m) ((uint16_t)((m)&0x7FFF))
#define MSG_REGDATA (0)
#define MSG_PTRDATA (0x8000)
#define MSG_PTR(m) (m.arg0)
#define MSG_SIZE(m) (m.arg1)
#define MSG_MAGIC(m) (m.arg2)
#define MSG_ISREG(m) (!((m)&MSG_PTRDATA))
#define MSG_ISPTR(m) ((m)&MSG_PTRDATA)
//eg.:
//pid_t child = fork();
//msg->evt = MSG_DEST(child) | MSG_FUNC(anynumber) | MSG_PTRDATA

typedef struct {
    uint64_t screen_ptr;    // screen virtual address ("SYS" and "UI")
    uint64_t fb_ptr;        // framebuffer virtual address ("SYS" only)
    uint64_t fb_width;      // framebuffer width
    uint64_t fb_height;     // framebuffer height
    uint64_t fb_scanline;   // framebuffer line size
    uint64_t quantum;       // max time a task can allocate CPU 1/quantum
    uint64_t quantumcnt;    // total number of task switches
    uint64_t freq;          // timer freqency, task switch at freq/quantum
    uint64_t ticks[2];      // overall jiffies counter
    uint64_t timestamp_s;   // UTC timestamp
    uint64_t timestamp_ns;  // UTC timestamp nanosec fraction
    uint64_t srand[4];      // random seed bits
    uint64_t debug;         // debug flags (see env.h)
    uint8_t display;        // display type (see env.h)
    uint8_t fps;            // maximum frame per second
    uint8_t rescueshell;    // rescue shell requested flag
} __attribute__((packed)) sysinfo_t;
#endif

#define sysinfo_screen 0
#define sysinfo_fbuf 8
#define sysinfo_fb_w 16
#define sysinfo_fb_h 24
#define sysinfo_fb_s 32

#endif /* sys/osZ.h */
