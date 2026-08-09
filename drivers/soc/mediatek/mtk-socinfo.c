// SPDX-License-Identifier: GPL-2.0
#include <linux/err.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>

static struct soc_device_attribute *soc_dev_attr;
static struct soc_device *soc_dev;

static int __init mediatek_socinfo_init(void)
{
	struct device_node *root;
	int ret;

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr)
		return -ENOMEM;

	soc_dev_attr->machine = "Mediatek";
	root = of_find_node_by_path("/");
	if (root) {
		of_property_read_string(root, "model", &soc_dev_attr->soc_id);
		of_node_put(root);
	}

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev)) {
		ret = PTR_ERR(soc_dev);
		kfree(soc_dev_attr);
		soc_dev_attr = NULL;
		return ret;
	}

	pr_info("MediaTek SoC: %s\n",
		soc_dev_attr->soc_id ?: "unknown");
	return 0;
}
device_initcall(mediatek_socinfo_init);
