```c
	/*
	 * The root irq controller is the one without an interrupt-parent.
	 * That one goes first, followed by the controllers that reference it,
	 * followed by the ones that reference the 2nd level controllers, etc.
	 */
	while (!list_empty(&intc_desc_list)) {
		/*
		 * Process all controllers with the current 'parent'.
		 * First pass will be looking for NULL as the parent.
		 * The assumption is that NULL parent means a root controller.
         * list_for_each_entry_safe 循环中只处理 parent node
		 */
		list_for_each_entry_safe(desc, temp_desc, &intc_desc_list, list) {
			const struct of_device_id *match;
			int ret;
			of_irq_init_cb_t irq_init_cb;
            
            /* 先处理 root interupt controller block */
			if (desc->interrupt_parent != parent)
				continue;
            
            /* 从 intc_desc_list 中删除即将处理的 node */
			list_del(&desc->list);
			match = of_match_node(matches, desc->dev);
			if (WARN(!match->data,
			    "of_irq_init: no init function for %s\n",
			    match->compatible)) {
				kfree(desc);
				continue;
			}

			pr_debug("of_irq_init: init %s @ %p, parent %p\n",
				 match->compatible,
				 desc->dev, desc->interrupt_parent);
			irq_init_cb = (of_irq_init_cb_t)match->data;
            /* 通过 irq_init_cb 函数指针调用 match->data 指向的中断控制器驱动的 init 函数*/
			ret = irq_init_cb(desc->dev, desc->interrupt_parent);
			if (ret) {
				kfree(desc);
				continue;
			}

			/*
			 * This one is now set up; add it to the parent list so
			 * its children can get processed in a subsequent pass.
			 */
            /* 将处理成功的 node 加入 intc_parent_list */
			list_add_tail(&desc->list, &intc_parent_list);
		}

		/* Get the next pending parent that might have children */
        desc = list_first_entry_or_null(&intc_parent_list,
						typeof(*desc), list);
        /* 如果没有找到 parent node，直接 break 出 while */
		if (!desc) {
			pr_err("of_irq_init: children remain, but no parents\n");
			break;
		}
        /* 置空 intc_parent_list */
		list_del(&desc->list);
		parent = desc->dev;
		kfree(desc);
	}
```

```c    
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_first_entry(head, typeof(*pos), member),	\
		n = list_next_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = list_next_entry(n, member))
```