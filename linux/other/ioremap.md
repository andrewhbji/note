```c
void __iomem *ioremap(resource_size_t res_cookie, size_t size)
{
	return arch_ioremap_caller(res_cookie, size, MT_DEVICE,
				   __builtin_return_address(0));
}
```

```c
void __iomem * (*arch_ioremap_caller)(phys_addr_t, size_t,
				      unsigned int, void *) =
	__arm_ioremap_caller;
```

```c
void __iomem *__arm_ioremap_caller(phys_addr_t phys_addr, size_t size,
	unsigned int mtype, void *caller)
{
	phys_addr_t last_addr;
 	unsigned long offset = phys_addr & ~PAGE_MASK;
 	unsigned long pfn = __phys_to_pfn(phys_addr);

 	/*
 	 * Don't allow wraparound or zero size
	 */
	last_addr = phys_addr + size - 1;
	if (!size || last_addr < phys_addr)
		return NULL;

	return __arm_ioremap_pfn_caller(pfn, offset, size, mtype,
			caller);
}
```