```c
tatic u8 gic_get_cpumask(struct gic_chip_data *gic)
{
    void __iomem *base = gic_data_dist_base(gic);
    u32 mask, i;
    
    /* 循环读取 0 - 7 号 GIC_DIST_TARGET 寄存器的值，只要一个寄存器可有效访问，就取该寄存器的高八位作为 cpumask */
    for (i = mask = 0; i < 32; i += 4) {    
        mask = readl_relaxed(base + GIC_DIST_TARGET + i);   
        mask |= mask >> 16;
        mask |= mask >> 8;
        if (mask)   /* mask 为 0 只有一种情况，该 GIC_DIST_TARGETi 寄存器不能被当前 CPU 访问 */
            break;
    }

    return mask; /* 返 u8 需要取 mask 的高八位 */
} 
```