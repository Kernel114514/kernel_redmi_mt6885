// SPDX-License-Identifier: GPL-2.0
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/mi_memory.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

struct mi_memory_data {
	struct class *class;
	struct device *device;
	int major;
};

static struct mi_memory_data mi_memory;

static ssize_t total_heaps_kb_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%llu\n",
		(unsigned long long)ion_total_heaps_kb());
}
static DEVICE_ATTR_RO(total_heaps_kb);

static ssize_t total_pools_kb_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%llu\n",
		(unsigned long long)ion_total_pools_kb());
}
static DEVICE_ATTR_RO(total_pools_kb);

static struct attribute *mi_memory_attrs[] = {
	&dev_attr_total_heaps_kb.attr,
	&dev_attr_total_pools_kb.attr,
	NULL,
};

static const struct attribute_group mi_memory_group = {
	.attrs = mi_memory_attrs,
};

static int proc_transaction_show(struct seq_file *m, void *unused)
{
	return binder_proc_transaction_show(m, (pid_t)(unsigned long)m->private);
}

static int proc_transaction_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_transaction_show, PDE_DATA(inode));
}

static const struct file_operations proc_transaction_fops = {
	.owner = THIS_MODULE,
	.open = proc_transaction_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations mi_memory_fops = {
	.owner = THIS_MODULE,
};

static int __init mi_memory_sysfs_init(void)
{
	int ret;

	mi_memory.class = class_create(THIS_MODULE, "mi_memory");
	if (IS_ERR(mi_memory.class))
		return PTR_ERR(mi_memory.class);

	mi_memory.major = register_chrdev(0, "mi_memory_module",
					  &mi_memory_fops);
	if (mi_memory.major < 0) {
		ret = mi_memory.major;
		goto err_class;
	}

	mi_memory.device = device_create(mi_memory.class, NULL,
		MKDEV(mi_memory.major, 1), NULL, "mi_memory_device");
	if (IS_ERR(mi_memory.device)) {
		ret = PTR_ERR(mi_memory.device);
		goto err_chrdev;
	}

	ret = sysfs_create_group(&mi_memory.device->kobj, &mi_memory_group);
	if (ret)
		goto err_device;

	if (!proc_create_data("binder_proc_transaction", 0555, NULL,
			      &proc_transaction_fops, NULL)) {
		ret = -ENOMEM;
		goto err_group;
	}

	return 0;

err_group:
	sysfs_remove_group(&mi_memory.device->kobj, &mi_memory_group);
err_device:
	device_destroy(mi_memory.class, MKDEV(mi_memory.major, 1));
err_chrdev:
	unregister_chrdev(mi_memory.major, "mi_memory_module");
err_class:
	class_destroy(mi_memory.class);
	return ret;
}
module_init(mi_memory_sysfs_init);

static void __exit mi_memory_sysfs_exit(void)
{
	remove_proc_entry("binder_proc_transaction", NULL);
	sysfs_remove_group(&mi_memory.device->kobj, &mi_memory_group);
	device_destroy(mi_memory.class, MKDEV(mi_memory.major, 1));
	unregister_chrdev(mi_memory.major, "mi_memory_module");
	class_destroy(mi_memory.class);
}
module_exit(mi_memory_sysfs_exit);

MODULE_LICENSE("GPL v2");
