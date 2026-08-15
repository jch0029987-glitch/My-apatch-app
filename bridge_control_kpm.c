// bridge_control_kpm.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeremy");
MODULE_DESCRIPTION("APatch Hardened Control Hub KPM");
MODULE_VERSION("1.4");

static struct kobject *bridge_kobj = NULL;
static int hub_status_value = 1; 

// Mutex lock to prevent race conditions during concurrent sysfs read/write ops
static DEFINE_MUTEX(bridge_lock);

static ssize_t status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    int ret;

    if (!buf) {
        pr_err_ratelimited("BridgeHub: status_show passed a NULL destination buffer\n");
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&bridge_lock))
        return -ERESTARTSYS;

    ret = snprintf(buf, PAGE_SIZE, "%d\n", hub_status_value);
    
    mutex_unlock(&bridge_lock);
    return ret;
}

static ssize_t status_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    int ret;
    int parsed_value;

    if (!buf) {
        pr_err_ratelimited("BridgeHub: status_store passed a NULL source buffer\n");
        return -EINVAL;
    }

    // Input bounds validation to prevent overflows
    if (count == 0 || count > 16) {
        pr_warn_ratelimited("BridgeHub: Rejected invalid input size (%zu bytes)\n", count);
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&bridge_lock))
        return -ERESTARTSYS;

    ret = kstrtoint_from_user(buf, count, 10, &parsed_value);
    if (ret < 0) {
        mutex_unlock(&bridge_lock);
        return ret; 
    }

    // Range clamping
    if (parsed_value < 0 || parsed_value > 255) {
        mutex_unlock(&bridge_lock);
        return -ERANGE;
    }

    hub_status_value = parsed_value;
    pr_info("BridgeHub: State safely updated to %d by UID %u\n", hub_status_value, from_kuid(&init_user_ns, current_uid()));

    mutex_unlock(&bridge_lock);
    return count;
}

static struct kobj_attribute hub_status_attr = __ATTR(status, 0664, status_show, status_store);

static int __init bridge_hub_init(void) {
    int error = 0;
    pr_info("BridgeHub: Initializing hardened APatch control module\n");

    // Create under /sys/kernel/bridge_hub
    bridge_kobj = kobject_create_and_add("bridge_hub", kernel_kobj);
    if (!bridge_kobj) {
        pr_err("BridgeHub: Critical - Failed memory allocation for kobject\n");
        return -ENOMEM;
    }

    error = sysfs_create_file(bridge_kobj, &hub_status_attr.attr);
    if (error) {
        pr_err("BridgeHub: Critical - Failed to create sysfs node, error: %d\n", error);
        kobject_put(bridge_kobj);
        bridge_kobj = NULL;
        return error;
    }

    pr_info("BridgeHub: Loaded successfully with active protections\n");
    return 0;
}

static void __exit bridge_hub_exit(void) {
    pr_info("BridgeHub: Executing safe teardown\n");
    
    if (mutex_lock_interruptible(&bridge_lock) == 0) {
        if (bridge_kobj) {
            kobject_put(bridge_kobj);
            bridge_kobj = NULL;
            pr_info("BridgeHub: Teardown complete, references cleared\n");
        }
        mutex_unlock(&bridge_lock);
    }
}

module_init(bridge_hub_init);
module_exit(bridge_hub_exit);
