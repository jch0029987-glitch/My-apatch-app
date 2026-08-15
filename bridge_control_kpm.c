#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeremy");
MODULE_DESCRIPTION("APatch Extreme-Safety Control Hub KPM");
MODULE_VERSION("1.2");

static struct kobject *bridge_kobj = NULL;
static int hub_status_value = 1; 

// Read handler with structural null-pointer defense
static ssize_t status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    if (!buf) {
        pr_err("BridgeHub: Fatal - status_show passed a NULL destination buffer\n");
        return -EINVAL;
    }
    
    return snprintf(buf, PAGE_SIZE, "%d\n", hub_status_value);
}

// Write handler with strict user-space boundary checking and parsing safety
static ssize_t status_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    int ret;
    int parsed_value;

    if (!buf) {
        pr_err("BridgeHub: Fatal - status_store passed a NULL source buffer\n");
        return -EINVAL;
    }

    if (count == 0 || count > 16) {
        pr_warn("BridgeHub: Rejected input size overflow/underflow (%zu bytes)\n", count);
        return -EINVAL;
    }

    // Safely copy and parse string crossing the user-to-kernel boundary
    ret = kstrtoint_from_user(buf, count, 10, &parsed_value);
    if (ret < 0) {
        pr_err("BridgeHub: Failed parsing user string to integer: %d\n", ret);
        return ret; 
    }

    // Explicit state boundary enforcement
    if (parsed_value < 0 || parsed_value > 255) {
        pr_warn("BridgeHub: Input value %d out of safe range (0-255)\n", parsed_value);
        return -ERANGE;
    }

    hub_status_value = parsed_value;
    pr_info("BridgeHub: State safely updated to %d\n", hub_status_value);

    return count;
}

static struct kobj_attribute hub_status_attr = __ATTR(status, 0664, status_show, status_store);

static int __init bridge_hub_init(void) {
    int error = 0;
    pr_info("BridgeHub: Initializing hard-guarded kernel control module\n");

    // 1. Allocate kobject safely under kernel root
    bridge_kobj = kobject_create_and_add("bridge_hub", kernel_kobj);
    if (!bridge_kobj) {
        pr_err("BridgeHub: Critical - Failed memory allocation for kobject\n");
        return -ENOMEM;
    }

    // 2. Create target file node with roll-back safety on failure
    error = sysfs_create_file(bridge_kobj, &hub_status_attr.attr);
    if (error) {
        pr_err("BridgeHub: Critical - Failed to create sysfs node, error: %d\n", error);
        kobject_put(bridge_kobj);
        bridge_kobj = NULL;
        return error;
    }

    pr_info("BridgeHub: Module loaded and verified safely\n");
    return 0;
}

static void __exit bridge_hub_exit(void) {
    pr_info("BridgeHub: Executing safe teardown\n");
    
    if (bridge_kobj) {
        kobject_put(bridge_kobj);
        bridge_kobj = NULL;
        pr_info("BridgeHub: Teardown complete, references cleared\n");
    } else {
        pr_warn("BridgeHub: Exit invoked but kobject was already NULL\n");
    }
}

module_init(bridge_hub_init);
module_exit(bridge_hub_exit);
