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
#include "libc/limits.h"
#include "libc/runtime/ezmap.internal.h"
#include "libc/runtime/runtime.h"
#include "libc/sysv/consts/map.h"
#include "libc/sysv/consts/o.h"
#include "libc/sysv/consts/prot.h"

hidden int MapFileRead(const char *filename, struct MappedFile *mf) {
  mf->addr = MAP_FAILED;
  if ((mf->fd = open(filename, O_RDONLY)) != -1 &&
      (mf->size = getfiledescriptorsize(mf->fd)) < INT_MAX &&
      (mf->addr = mf->size ? mmap(NULL, mf->size, PROT_READ,
                                  MAP_PRIVATE | MAP_POPULATE, mf->fd, 0)
                           : NULL) != MAP_FAILED) {
    return 0;
  } else {
    UnmapFile(mf);
    return -1;
  }
}

hidden int UnmapFile(struct MappedFile *mf) {
  int rc;
  rc = 0;
  if (mf->addr && mf->addr != MAP_FAILED) {
    rc |= munmap(mf->addr, mf->size);
    mf->addr = MAP_FAILED;
  }
  if (mf->fd != -1) {
    rc |= close(mf->fd);
    mf->fd = -1;
  }
  mf->size = 0;
  return rc;
}
