// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include <cstdint>
using OrbisKernelUseconds = uint32_t;
struct MockNativeSemaphore;
using OrbisKernelSema = MockNativeSemaphore *;
struct OrbisKernelSemaOptParam;
#define ORBIS_KERNEL_SEMA_ATTR_TH_FIFO 1
#define ORBIS_KERNEL_ERROR_EINVAL 0x80020016u
#define ORBIS_KERNEL_ERROR_EBUSY 0x80020010u
#define ORBIS_KERNEL_ERROR_EAGAIN 0x80020023u
#define ORBIS_KERNEL_ERROR_ETIMEDOUT 0x8002003cu
#define ORBIS_KERNEL_ERROR_EINTR 0x80020004u
int32_t sceKernelCreateSema(OrbisKernelSema *, const char *, uint32_t, int32_t,
	int32_t, const OrbisKernelSemaOptParam *);
int32_t sceKernelDeleteSema(OrbisKernelSema);
int32_t sceKernelSignalSema(OrbisKernelSema, int);
int32_t sceKernelWaitSema(OrbisKernelSema, int32_t, OrbisKernelUseconds *);
int32_t sceKernelPollSema(OrbisKernelSema, int);

uint64_t sceKernelGetProcessTime();
