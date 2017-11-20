/*
 * Copyright (c) 2011 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HAX_INTERFACE_H_
#define HAX_INTERFACE_H_

/*
 * The interface to QEMU, notice:
 * 1) not include any file other than top level include
 * 2) will be shared by QEMU and kernel
 */

#include "hax_types.h"

#ifdef __MACH__
#include "darwin/hax_interface_mac.h"
#endif

#ifdef __WINNT__
#include "windows/hax_interface_windows.h"
#endif

#include "vcpu_state.h"

struct vmx_msr {
    uint64_t entry;
    uint64_t value;
} PACKED;

/* fx_layout has 3 formats table 3-56, 512bytes */
struct fx_layout {
    uint16  fcw;
    uint16  fsw;
    uint8   ftw;
    uint8   res1;
    uint16  fop;
    union {
        struct {
            uint32  fip;
            uint16  fcs;
            uint16  res2;
        };
        uint64  fpu_ip;
    };
    union {
        struct {
            uint32  fdp;
            uint16  fds;
            uint16  res3;
        };
        uint64  fpu_dp;
    };
    uint32  mxcsr;
    uint32  mxcsr_mask;
    uint8   st_mm[8][16];
    uint8   mmx_1[8][16];
    uint8   mmx_2[8][16];
    uint8   pad[96];
} ALIGNED(16);

/*
 * TODO: Fixed array is stupid, but it makes Mac support a bit easier, since we
 * can avoid the memory map or copyin staff. We need to fix it in future.
 */

#define HAX_MAX_MSR_ARRAY 0x20
struct hax_msr_data {
    uint16_t nr_msr;
    uint16_t done;
    uint16_t pad[2];
    struct vmx_msr entries[HAX_MAX_MSR_ARRAY];
} PACKED;

#define HAX_IO_OUT 0
#define HAX_IO_IN  1

/* The area to communicate with device model */
struct hax_tunnel {
    uint32_t _exit_reason;
    uint32_t pad0;
    uint32_t _exit_status;
    uint32_t user_event_pending;
    int ready_for_interrupt_injection;
    int request_interrupt_window;

    union {
        struct {
            uint8_t _direction;
            uint8_t _df;
            uint16_t _size;
            uint16_t _port;
            uint16_t _count;
            /* Followed owned by HAXM, QEMU should not touch them */
            /* bit 1 is 1 means string io */
            uint8_t _flags;
            uint8_t _pad0;
            uint16_t _pad1;
            uint32_t _pad2;
            vaddr_t _vaddr;
        } io;
        struct {
            paddr_t gla;
        } mmio;
        struct {
            paddr_t dummy;
        } state;
    };
} PACKED;

struct hax_fastmmio {
    paddr_t gpa;
    union {
        uint64_t value;
        paddr_t gpa2;  /* since API v4 */
    };
    uint8_t size;
    uint8_t direction;
    uint16_t reg_index;  /* obsolete */
    uint32_t pad0;
    uint64_t _cr0;
    uint64_t _cr2;
    uint64_t _cr3;
    uint64_t _cr4;
} PACKED;

struct hax_module_version {
    uint32_t compat_version;
    uint32_t cur_version;
} PACKED;

#define HAX_CAP_STATUS_WORKING     0x0
#define HAX_CAP_STATUS_NOTWORKING  0x1

#define HAX_CAP_FAILREASON_VT      0x1
#define HAX_CAP_FAILREASON_NX      0x2

struct hax_capabilityinfo {
    /*
     * bit 0: 1 - working, 0 - not working, possibly because NT/NX disabled
     * bit 1: 1 - memory limitation working, 0 - no memory limitation
     */
    uint16_t wstatus;
    /*
     * valid when not working
     * bit0: VT not enabeld
     * bit1: NX not enabled
     */
    /*
     * valid when working
     * bit0: EPT enabled
     * bit1: fastMMIO
     */
    uint16_t winfo;
    uint32_t win_refcount;
    uint64_t mem_quota;
} PACKED;

struct hax_tunnel_info {
    uint64_t va;
    uint64_t io_va;
    uint16_t size;
    uint16_t pad[3];
} PACKED;

struct hax_set_memlimit {
    uint8_t enable_memlimit;
    uint8_t pad[7];
    uint64_t memory_limit;
} PACKED;

struct hax_alloc_ram_info {
    uint32_t size;
    uint32_t pad;
    uint64_t va;
} PACKED;

#define HAX_RAM_INFO_ROM     0x01  // read-only
#define HAX_RAM_INFO_INVALID 0x80  // unmapped, usually used for MMIO

struct hax_set_ram_info {
    uint64_t pa_start;
    uint32_t size;
    uint8_t flags;
    uint8_t pad[3];
    uint64_t va;
} PACKED;

/* This interface is support only after API version 2 */
struct hax_qemu_version {
    /* Current API version in QEMU*/
    uint32_t cur_version;
    /* The least API version supported by QEMU */
    uint32_t least_version;
} PACKED;

#endif  // HAX_INTERFACE_H_