/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2020 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ This program is free software; you can redistribute it and/or modify         │
│ it under the terms of the GNU General Public License as published by         │
│ the Free Software Foundation; version 2 of the License.                      │
│                                                                              │
│ This program is distributed in the hope that it will be useful, but          │
│ WITHOUT ANY WARRANTY; without even the implied warranty of                   │
│ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU             │
│ General Public License for more details.                                     │
│                                                                              │
│ You should have received a copy of the GNU General Public License            │
│ along with this program; if not, write to the Free Software                  │
│ Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA                │
│ 02110-1301 USA                                                               │
╚─────────────────────────────────────────────────────────────────────────────*/
#include "libc/bits/safemacros.h"
#include "libc/calls/calls.h"
#include "libc/calls/internal.h"
#include "libc/dce.h"
#include "libc/nt/winsock.h"
#include "libc/sock/internal.h"
#include "libc/str/str.h"
#include "libc/sysv/errfuns.h"

static textwindows ssize_t sendfile$linux2nt(int outfd, int infd,
                                             int64_t *inout_opt_inoffset,
                                             size_t uptobytes) {
  struct NtOverlapped Overlapped;
  struct NtOverlapped *lpOverlapped;
  if (!__isfdkind(outfd, kFdSocket) || !__isfdkind(outfd, kFdFile))
    return ebadf();
  if (inout_opt_inoffset) {
    memset(&Overlapped, 0, sizeof(Overlapped));
    Overlapped.Pointer = (void *)(intptr_t)(*inout_opt_inoffset);
    lpOverlapped = &Overlapped;
  } else {
    lpOverlapped = NULL;
  }
  /* TODO(jart): Fetch this on a per-socket basis via GUID. */
  if (TransmitFile(g_fds.p[outfd].handle, g_fds.p[infd].handle, uptobytes, 0,
                   lpOverlapped, NULL, 0)) {
    return uptobytes;
  } else {
    return __winsockerr();
  }
}

static ssize_t sendfile$linux2netflix(int outfd, int infd,
                                      int64_t *inout_opt_inoffset,
                                      size_t uptobytes) {
  int sendfile$netflix(int32_t infd, int32_t outfd, int64_t offset,
                       size_t nbytes, const void *opt_hdtr,
                       int64_t *out_opt_sbytes,
                       int32_t flags) asm("sendfile$sysv") hidden;
  int rc;
  int64_t offset, sbytes;
  if (inout_opt_inoffset) {
    offset = *inout_opt_inoffset;
  } else if ((offset = lseek$sysv(infd, 0, SEEK_CUR)) == -1) {
    return -1;
  }
  if ((rc = sendfile$netflix(infd, outfd, offset, uptobytes, NULL, &sbytes,
                             0)) != -1) {
    if (inout_opt_inoffset) *inout_opt_inoffset += sbytes;
    return sbytes;
  } else {
    return -1;
  }
}

/**
 * Transfers data from file to network.
 *
 * @param outfd needs to be a socket
 * @param infd needs to be a file
 * @param inout_opt_inoffset may be specified for pread()-like behavior
 * @param uptobytes is usually the number of bytes remaining in file; it
 *     can't exceed INT_MAX-1; some platforms block until everything's
 *     sent, whereas others won't; zero isn't allowed
 * @return number of bytes transmitted which may be fewer than requested
 * @see copy_file_range() for file ↔ file
 * @see splice() for fd ↔ pipe
 */
ssize_t sendfile(int outfd, int infd, int64_t *inout_opt_inoffset,
                 size_t uptobytes) {
  if (!uptobytes) return einval();
  if (uptobytes > 0x7ffffffe /* Microsoft's off-by-one */) return eoverflow();
  if (IsModeDbg() && uptobytes > 1) uptobytes >>= 1;
  if (IsLinux()) {
    return sendfile$sysv(outfd, infd, inout_opt_inoffset, uptobytes);
  } else if (IsFreebsd() || IsXnu()) {
    return sendfile$linux2netflix(outfd, infd, inout_opt_inoffset, uptobytes);
  } else if (IsWindows()) {
    return sendfile$linux2nt(outfd, infd, inout_opt_inoffset, uptobytes);
  } else {
    return copyfd(infd, inout_opt_inoffset, outfd, NULL, uptobytes, 0);
  }
}
