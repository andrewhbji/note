#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>

static int func1(void)
{
        printk(KERN_ALERT "In Func: %s...\n",__func__);
        return 0;
}

EXPORT_SYMBOL(func1);

static int __init hello_init(void)
{
        printk(KERN_ALERT "Module 1,Init!\n");
        return 0;
}

static void __exit hello_exit(void)
{
        printk(KERN_ALERT "Module 1,Exit!\n");
}

module_init(hello_init);
module_exit(hello_exit);