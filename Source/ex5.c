#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h> // work queue API
#include <linux/printk.h>    // pr_info
#include <linux/jiffies.h>   // jiffies, HZ

// Forward declaration needed
static struct delayed_work ex5_delayed_work;

// Delayed work handler - reschedules itself!
static void ex5_work_handler(struct work_struct *work)
{
    static int count = 0;
    // We need the delayed_work struct to reschedule, not just work_struct
    // struct delayed_work *dwork = container_of(work, struct delayed_work, work);
    // Actually, DECLARE_DELAYED_WORK gives us the instance directly.

    pr_info("Ex5 Repetitive Delayed Work: Handler execution #%d\n", ++count);

    // Reschedule myself for 1 second later
    if (count < 5) { // Limit for testing
        pr_info("Ex5 Repetitive Delayed Work: Rescheduling for 1 second later.\n");
        // Use the SAME delayed_work structure instance
        schedule_delayed_work(&ex5_delayed_work, HZ); // HZ = 1 second delay
    } else {
        pr_info("Ex5 Repetitive Delayed Work: Reached limit, stopping rescheduling.\n");
    }
}

// Statically declare the delayed work item
static DECLARE_DELAYED_WORK(ex5_delayed_work, ex5_work_handler);

static int __init ex5_init(void)
{
    pr_info("Ex5 Module: Loading...\n");

    // Schedule the first execution (2 seconds delay initially)
    pr_info("Ex5 Module: Scheduling first delayed work run (2 seconds delay).\n");
    schedule_delayed_work(&ex5_delayed_work, 2 * HZ);

    return 0; // Success
}

static void __exit ex5_exit(void)
{
    pr_info("Ex5 Module: Exiting...\n");

    // CRITICAL: Cancel the delayed work. This prevents it from running
    // or rescheduling after the module code is gone. Waits if running.
    if (cancel_delayed_work_sync(&ex5_delayed_work)) {
        pr_info("Ex5 Module: Repetitive delayed work was pending and is now cancelled.\n");
    } else {
        pr_info("Ex5 Module: Repetitive delayed work was not pending (already run or finished).\n");
    }

    pr_info("Ex5 Module: Unloaded.\n");
}

module_init(ex5_init);
module_exit(ex5_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver Jędrzejczyk");
MODULE_DESCRIPTION("Exercise 5: Automatically repetitive delayed work");
MODULE_VERSION("1.0");