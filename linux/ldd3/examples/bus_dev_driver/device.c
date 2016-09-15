#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>

MODULE_AUTHOR("yinjiabin");
MODULE_LICENSE("Dual BSD/GPL");

extern struct device my_bus;
extern struct bus_type my_bus_type;

/* Why need this ?*/
static void my_dev_release(struct device *dev)
{

}

struct device my_dev = {
        .init_name = "my_dev",
        .bus = &my_bus_type,   //指明设备所属的总线
        .parent = &my_bus,     //该设备的父设备，总线也是一种设备
        .release = my_dev_release,
};

static ssize_t mydev_show(struct device *dev,struct device_attribute *attr, char *buf)
{
        return sprintf(buf, "%s\n", "This is my device!");
}

static DEVICE_ATTR(dev, S_IRUGO, mydev_show, NULL);

static int __init my_device_init(void)
{
        int ret = 0;

        /* 初始化设备 */
        ///strncpy(my_dev->kobj.name, "my_dev", strlen(my_dev->name));

        /*注册设备*/
        device_register(&my_dev);

        /*创建属性文件*/
        device_create_file(&my_dev, &dev_attr_dev);

        return ret;

}

static void my_device_exit(void)
{
        device_unregister(&my_dev);
}

module_init(my_device_init);
module_exit(my_device_exit);
