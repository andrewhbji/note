```c
struct device_node *of_irq_find_parent(struct device_node *child)
{
	struct device_node *p;
	const __be32 *parp;

	if (!of_node_get(child))
		return NULL;

	do {
        /* 获取 interrupt-parent 属性*/
		parp = of_get_property(child, "interrupt-parent", NULL);
		if (parp == NULL)
			p = of_get_parent(child);
		else {
			if (of_irq_workarounds & OF_IMAP_NO_PHANDLE)
				p = of_node_get(of_irq_dflt_pic);
			else
                /* 根据 parp 属性获取父节点*/
				p = of_find_node_by_phandle(be32_to_cpup(parp));
		}
		of_node_put(child);
		child = p;
        /* 父节点存在，且父节点没有设置#interrupt-cells属性，则继续循环 */
	} while (p && of_get_property(p, "#interrupt-cells", NULL) == NULL);

	return p;
}
```