#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h> // work queue API
#include <linux/printk.h>    // pr_info

// Simple work handler
static void ex4_work_handler(struct work_struct *work)
{
    // Note: This might not even get called if loading fails
    pr_info("Ex4 Work Handler: Executing!\n");
}

// Declare a work item statically
static DECLARE_WORK(ex4_work, ex4_work_handler);

static int __init ex4_init(void)
{
    pr_info("Ex4 Non-GPL Module: Attempting to load...\n");

    // Try to schedule work on the default work queue
    // schedule_work is GPL-exported, this will likely cause issues
    if (schedule_work(&ex4_work)) {
        pr_info("Ex4 Non-GPL Module: Work scheduled successfully (unexpected?).\n");
    } else {
        pr_info("Ex4 Non-GPL Module: Work was already scheduled?\n");
    }

    pr_info("Ex4 Non-GPL Module: Loaded (or seemed to load).\n");
    return 0;
}

static void __exit ex4_exit(void)
{
    pr_info("Ex4 Non-GPL Module: Exiting...\n");

    // Attempt to cancel the work - might also fail if schedule_work failed
    // cancel_work_sync is also GPL-exported
    if (cancel_work_sync(&ex4_work)) {
         pr_info("Ex4 Non-GPL Module: Work cancelled successfully.\n");
    } else {
         pr_info("Ex4 Non-GPL Module: Work was not pending or already done.\n");
    }

    pr_info("Ex4 Non-GPL Module: Unloaded.\n");
}

module_init(ex4_init);
module_exit(ex4_exit);


MODULE_LICENSE("Proprietary");
// MODULE_LICENSE("Dual BSD/GPL"); // Also try this
MODULE_AUTHOR("Oliver Jędrzejczyk");
MODULE_DESCRIPTION("Exercise 4: Test work queue with non-GPL license");
MODULE_VERSION("1.0");