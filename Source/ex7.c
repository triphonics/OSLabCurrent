#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h> // work queue API
#include <linux/printk.h>    // pr_info
#include <linux/jiffies.h>   // HZ
#include <linux/slab.h>      // kmalloc/kfree (if needed for data)

// Handlers are the same as Listing 2 / Exercise 5
static void normal_work_handler(struct work_struct *work)
{
    pr_info("Ex7 Normal Work Handler: Hi! I'm handler of normal work!\n");
}

static void delayed_work_handler(struct work_struct *work)
{
    pr_info("Ex7 Delayed Work Handler: Hi! I'm handler of delayed work!\n");
}

// Declare work items statically
static DECLARE_WORK(normal_work, normal_work_handler);
static DECLARE_DELAYED_WORK(delayed_work, delayed_work_handler);

static int __init ex7_init(void)
{
    pr_info("Ex7 Module (Default WQ): Loading...\n");

    // No need to create a workqueue!

    // Schedule normal work on the default workqueue
    if (!schedule_work(&normal_work)) {
        pr_info("Ex7: The normal work was already queued!\n");
    } else {
        pr_info("Ex7: Normal work scheduled on default queue.\n");
    }

    // Schedule delayed work on the default workqueue (delay 3 seconds)
    if (!schedule_delayed_work(&delayed_work, 3 * HZ)) {
        pr_info("Ex7: The delayed work was already queued!\n");
    } else {
         pr_info("Ex7: Delayed work scheduled on default queue (3 sec delay).\n");
    }

    return 0; // Success
}

static void __exit ex7_exit(void)
{
    pr_info("Ex7 Module (Default WQ): Exiting...\n");

    // Cancel the work items. Functions work regardless of queue type.
    pr_info("Ex7: Cancelling work items...\n");

    if (cancel_work_sync(&normal_work)) {
        pr_info("Ex7: The normal work has not been done yet! Cancelled.\n");
    } else {
         pr_info("Ex7: Normal work was not pending or already finished.\n");
    }

    if (cancel_delayed_work_sync(&delayed_work)) {
        pr_info("Ex7: The delayed work has not been done yet! Cancelled.\n");
    } else {
        pr_info("Ex7: Delayed work was not pending or already finished.\n");
    }

    // No need to destroy a workqueue!

    pr_info("Ex7 Module (Default WQ): Unloaded.\n");
}

module_init(ex7_init);
module_exit(ex7_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver Jędrzejczyk");
MODULE_DESCRIPTION("Exercise 7: Listing 2 using default work queue");
MODULE_VERSION("1.0");