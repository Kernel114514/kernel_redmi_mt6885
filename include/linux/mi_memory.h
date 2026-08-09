/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MI_MEMORY_H
#define _LINUX_MI_MEMORY_H

#include <linux/types.h>

struct seq_file;

u64 ion_total_heaps_kb(void);
u64 ion_total_pools_kb(void);
int binder_proc_transaction_show(struct seq_file *m, pid_t pid);

#endif
