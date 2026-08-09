// SPDX-License-Identifier: GPL-2.0
#include <asm/memory.h>
#include <asm/sections.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/utsname.h>

#define DEBUG_KINFO_MAGIC	0xcceeddff
#define DEBUG_KINFO_SIZE	0x1d4
#define DEBUG_KINFO_VERSION_OFF	0x84
#define DEBUG_KINFO_VERSION_LEN	0x40

struct debug_kinfo_blob {
	u8 data[DEBUG_KINFO_SIZE];
};

static void put_u32(struct debug_kinfo_blob *blob, size_t offset, u32 value)
{
	memcpy(blob->data + offset, &value, sizeof(value));
}

static void put_u64(struct debug_kinfo_blob *blob, size_t offset, u64 value)
{
	memcpy(blob->data + offset, &value, sizeof(value));
}

static u64 kimage_offset(const void *symbol)
{
	return (unsigned long)symbol - kimage_vaddr;
}

static int debug_kinfo_probe(struct platform_device *pdev)
{
	struct debug_kinfo_blob blob = {};
	struct device_node *memory;
	struct resource resource;
	void __iomem *base;
	u32 checksum = 0;
	size_t offset;
	int ret;

	memory = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!memory) {
		dev_warn(&pdev->dev, "no memory-region\n");
		return -ENODEV;
	}

	ret = of_address_to_resource(memory, 0, &resource);
	of_node_put(memory);
	if (ret)
		return ret;
	if (resource_size(&resource) < DEBUG_KINFO_SIZE)
		return -EINVAL;

	base = devm_ioremap(&pdev->dev, resource.start,
			    resource_size(&resource));
	if (!base)
		return -ENOMEM;

	put_u32(&blob, 0x00, DEBUG_KINFO_MAGIC);
	put_u32(&blob, 0x08, 0x01000000);
	put_u64(&blob, 0x10, 0x00ef003800400080ULL);
	put_u64(&blob, 0x18, kimage_offset(_text));
	put_u64(&blob, 0x28, kimage_offset(__exception_text_start));
	put_u64(&blob, 0x30, kimage_offset(_etext));
	put_u64(&blob, 0x38, kimage_offset(_stext));
	put_u64(&blob, 0x40, kimage_offset(_einittext));
	put_u64(&blob, 0x48, kimage_offset(__bss_start));
	put_u64(&blob, 0x50, kimage_offset(__bss_stop));
	put_u64(&blob, 0x58, kimage_offset(__start_rodata));
	put_u64(&blob, 0x60, kimage_offset(__end_rodata));
	put_u64(&blob, 0x68, kimage_offset(_sdata));
	put_u64(&blob, 0x70, kimage_offset(_edata));
	put_u64(&blob, 0x7c, kimage_offset(__per_cpu_start));
	strlcpy(blob.data + DEBUG_KINFO_VERSION_OFF,
		init_uts_ns.name.release, DEBUG_KINFO_VERSION_LEN);
	put_u64(&blob, 0x1c4, 0x0000018000000001ULL);
	put_u64(&blob, 0x1cc, 0x00000270000001d0ULL);

	for (offset = 8; offset < DEBUG_KINFO_SIZE; offset += sizeof(u32)) {
		u32 value;

		memcpy(&value, blob.data + offset, sizeof(value));
		checksum ^= value;
	}
	put_u32(&blob, 0x04, checksum);
	memcpy_toio(base, blob.data, sizeof(blob.data));

	return 0;
}

static const struct of_device_id debug_kinfo_of_match[] = {
	{ .compatible = "google,debug-kinfo" },
	{}
};
MODULE_DEVICE_TABLE(of, debug_kinfo_of_match);

static struct platform_driver debug_kinfo_driver = {
	.probe = debug_kinfo_probe,
	.driver = {
		.name = "debug-kinfo",
		.of_match_table = debug_kinfo_of_match,
	},
};
module_platform_driver(debug_kinfo_driver);

MODULE_LICENSE("GPL v2");
