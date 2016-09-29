```c
	for (dn = of_find_matching_node(NULL, matches); 
        dn;
        dn = of_find_matching_node(dn, matches)){
        /* 从匹配成功的节点中找到包含 interrupt-controller 的节点 */
		if (!of_find_property(np, "interrupt-controller", NULL) ||
				!of_device_is_available(np))
			continue;
		/*
		 * 填充 desc
		 */
		desc = kzalloc(sizeof(*desc), GFP_KERNEL);
		if (WARN_ON(!desc))
			goto err;

		desc->dev = np;
		desc->interrupt_parent = of_irq_find_parent(np);
        /* 将root interupt 的 parent 置空 */
		if (desc->interrupt_parent == np)
			desc->interrupt_parent = NULL;
        /* 将 dest->list 加入 intc_desc_list */
		list_add_tail(&desc->list, &intc_desc_list);
	}
```
    
```c    
static inline struct device_node *of_find_matching_node(
    struct device_node *from,
    const struct of_device_id *matches)
{
	return of_find_matching_node_and_match(from, matches, NULL);
}
```

```c
struct device_node *of_find_matching_node_and_match(struct device_node *from,
					const struct of_device_id *matches,
					const struct of_device_id **match)
{
	struct device_node *np;
	const struct of_device_id *m;
	unsigned long flags;

	if (match)
		*match = NULL;

	raw_spin_lock_irqsave(&devtree_lock, flags);
	np = from ? from->allnext : of_allnodes;
    /* 遍历指定范围的 device_node，每一个device_node 都用 matches 匹配，获取匹配结果 */
	for (; np; np = np->allnext) {
            /* 取出匹配成功的 matches 数组成员 */
		m = __of_match_node(matches, np);
		if (m && of_node_get(np)) {
			if (match)
				*match = m; /* match 指向匹配成功的 matches 数组成员 */
			break;  /* 匹配成功后退出循环 */
		}
	}
	of_node_put(from);
	raw_spin_unlock_irqrestore(&devtree_lock, flags);
	return np;  /* 返回匹配成功的 device_node */
}
```

```c
static
const struct of_device_id *__of_match_node(
    const struct of_device_id *matches,
    const struct device_node *node)
{
	const struct of_device_id *best_match = NULL;
	int score, best_score = 0;

	if (!matches)
		return NULL;
    /* 每一个 node 都使用所有 matches 匹配 ，得分最高的为最佳结果 */
	for (; matches->name[0] || matches->type[0] || matches->compatible[0]; matches++) {
		score = __of_device_is_compatible(node, matches->compatible,
						  matches->type, matches->name);
		if (score > best_score) {
			best_match = matches;
			best_score = score;
		}
	}
    /* 返回得分最高的 match */
	return best_match;
}
```

```c
static int __of_device_is_compatible(const struct device_node *device,
				     const char *compat, const char *type, const char *name)
{
	struct property *prop;
	const char *cp;
	int index = 0, score = 0;

	/* 优先匹配 compatible */
	if (compat && compat[0]) {
        /* 获取 device_node 的 compatible 属性 */
		prop = __of_find_property(device, "compatible", NULL);
        
		for (
                cp = of_prop_next_string(prop, NULL); /* 获取 compatible 属性值数组的首地址 */
                cp;
		        cp = of_prop_next_string(prop, cp), /* 将指针移动到 compatible 属性值数组的下一个位置 */
                index++) {
			if (of_compat_cmp(cp, compat, strlen(compat)) == 0) {
				score = INT_MAX/2 - (index << 2);   /* 计算得分，匹配次数越少，得分越高 */
				break; /* 匹配成功后直接退出循环 */
			}
		}
		if (!score)
			return 0;   /* 得分为0，直接返回 */
	}

	/* Matching type is better than matching name */
	if (type && type[0]) {
		if (!device->type || of_node_cmp(type, device->type))
			return 0;
		score += 2;
	}

	/* Matching name is a bit better than not */
	if (name && name[0]) {
		if (!device->name || of_node_cmp(name, device->name))
			return 0;
		score++;
	}

	return score;
}
```

```c
struct property {
	char	*name;
	int	length;
	void	*value;
	struct property *next;
	unsigned long _flags;
	unsigned int unique_id;
};
```

```c
static struct property *__of_find_property(
const struct device_node *np,
const char *name,
int *lenp)
{
	struct property *pp;

    /* np 为空则返回 */
	if (!np)
		return NULL;
    /* 从 *np 的属性集中取出 name 指定的属性 */    
	for (pp = np->properties; pp; pp = pp->next) {
		if (of_prop_cmp(pp->name, name) == 0) {
			if (lenp)
				*lenp = pp->length;
			break;
		}
	}

	return pp;
}
```

```c
const char *of_prop_next_string(struct property *prop, const char *cur)
{
	const void *curv = cur;
    /* prop 为空则返回 */
	if (!prop)
		return NULL;
    /* cur 为空则返回 prop->value */
	if (!cur)
		return prop->value;

	curv += strlen(cur) + 1; /* curv 指向 value 数组下一个元素的首位 */
    /* curv 越界，则返回 NULL */
	if (curv >= prop->value + prop->length)
		return NULL;

	return curv;
}
```