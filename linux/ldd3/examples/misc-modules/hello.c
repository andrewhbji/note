/*                                                     
 * $Id: hello.c,v 1.5 2004/10/26 03:32:21 corbet Exp $ 
 */                                                    
#include <linux/init.h>
#include <linux/module.h>
/*
 * 另一个特别的宏 (MODULE_LICENSE) 是用来告知内核, 该模块带有一个自由的许可证; 没有这样的说明, 在模块加载时内核会抱怨. 
 */
MODULE_LICENSE("Dual BSD/GPL");

static int hello_init(void)
{
	/*printk 函数在 Linux 内核中定义并且对模块可用; 它与标准 C 库函数 printf 的行为相似.*/
	/*字串 KERN_ALERT 是消息的优先级.*/
	printk(KERN_ALERT "Hello, world\n");
	return 0;
}

static void hello_exit(void)
{
	printk(KERN_ALERT "Goodbye, cruel world\n");
}

/*
 * moudle_init 和 module_exit 这几行使用了特别的内核宏来指出这两个函数的角色
 */
module_init(hello_init);
module_exit(hello_exit);
