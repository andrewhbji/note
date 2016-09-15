/*                                                     
 * $Id: hellopa.c,v 1.0 2015/09/23 07:02:43 gregkh Exp $ 
 */                                                    
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("Dual BSD/GPL");
#define ARRAYSIZE 10
/*                                                        
 * These lines, although not shown in the book,           
 * are needed to make hello.c run properly even when      
 * your kernel has version support enabled                
 */                                                       
                                                          

/*
 * 测试数组模块参数
 * 装载模块时指定数组参数,参数内容用逗号隔开,如:
 * $ insmod hellopa.ko array=1,2,3,4,5
 */
static int array[ARRAYSIZE];
int narr;

/*
 * 将narr的地址传递给module_param_array函数,接收数组长度
 */
module_param_array(array,int,&narr,S_IRUGO);

static int hello_init(void)
{
	int i;
	for (i = 0; i < narr; i++)
		printk(KERN_ALERT "array[%d] = %d\n", i, array[i]);
	return 0;
}

static void hello_exit(void)
{
	printk(KERN_ALERT "Goodbye, cruel world\n");
}

module_init(hello_init);
module_exit(hello_exit);