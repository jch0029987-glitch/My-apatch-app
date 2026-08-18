#include <kpmodule.h>  // Essential KernelPatch Macros and Definitions
#include <compiler.h>  // Compiler helpers for naked symbols and optimizations
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/uidgid.h>

// 1. Declare explicit KPM Metadata instead of standard LKM macros
KPM_NAME("bridge-control-kpm");
KPM_VERSION("1.4");
KPM_LICENSE("GPL");

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

    // FIX: sysfs provides a kernel-space buffer. Do not use _from_user variants.
    ret = kstrtoint(buf, 10, &parsed_value);
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

// 2. KPM Lifecycle Hook instead of __init bridge_hub_init
static int bridge_hub_init(int argc, char **argv) {
    int error = 0;
    pr_info("BridgeHub: Initializing hardened APatch control KPM\n");

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

// 3. KPM Lifecycle Exit function
static void bridge_hub_exit(void) {
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

// Register hooks via KPM specific macros
KPM_INIT(bridge_hub_init);
KPM_EXIT(bridge_hub_exit);
