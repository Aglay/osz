/*
 * pmm.c
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
 * @brief Physical Memory Manager architecture specific part
 */

#include "core.h"
#include "pmm.h"
#include "env.h"

extern void* __bss_start;

// Main scructure
OSZ_pmm __attribute__ ((section (".data"))) pmm;
// pointer to tmpmap
void __attribute__ ((section (".data"))) *kmap_tmp;

void* kalloc(int pages)
{
	//phys=pmm_alloc()
	//pmm_map(virt,phys)
    return 0;
}

void kfree(void* ptr)
{
}

void* pmm_alloc(int pages)
{
    return 0;
}

void pmm_init()
{
	// memory map provided by the loader
	MMapEnt *entry = (MMapEnt*)&bootboot.mmap;
	// our new free pages directory, pmm.entries
	OSZ_pmm_entry *fmem;
	uint num = (bootboot.size-128)/16;
	uint i;

	// allocate at least 2 pages for free memory entries
	// that's 2*4096/16 = 512 maximum fragments
	if(nrphymax<2)
		nrphymax=2;
	// initialize pmm structure
	pmm.magic = OSZ_PMM_MAGICH;
	pmm.size = 0;
	// first 3 pages are for temporary mappings, tmpmap and tmp2map
	// let's initialize them
	kmap_tmp = kmap_init();

	// buffers
	pmm.entries = fmem = (OSZ_pmm_entry*)((uint8_t*)&__bss_start + 3*__PAGESIZE);
	pmm.bss = (uint8_t*)&__bss_start;
	pmm.bss_end = (uint8_t*)&__bss_start + __PAGESIZE * (nrphymax+3);
	// this is a chicken and egg scenario. We need free memory to
	// store the free memory table...
	while(num>0) {
		if(MMapEnt_IsFree(entry) &&
		   MMapEnt_Size(entry)/__PAGESIZE>=nrphymax) {
				// map free physical pages to pmm.entries
				for(i=0;i<nrphymax;i++)
					kmap((uint64_t)((uint8_t*)fmem) + i*__PAGESIZE,
						(uint64_t)MMapEnt_Ptr(entry) + i*__PAGESIZE);
			// "preallocate" the memory for pmm.entries
			entry->ptr +=nrphymax*__PAGESIZE;
			entry->size-=nrphymax*__PAGESIZE;
			break;
		}
		num--;
		entry++;
	}

	// iterate through memory map again but this time
	// record free areas in pmm.entries
	num = (bootboot.size-128)/16;
	entry = (MMapEnt*)&bootboot.mmap;
	while(num>0) {
		if(MMapEnt_IsFree(entry)) {
			fmem->base = MMapEnt_Ptr(entry);
			fmem->size = MMapEnt_Size(entry)/__PAGESIZE;
			if(fmem->size!=0) {
				fmem++;
				pmm.size++;
			} else {
				fmem->base = 0;
			}
		}
		num--;
		entry++;
	}
}
