#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h> // tasklet API
#include <linux/printk.h>    // pr_info

// Forward declaration needed since the handler uses the tasklet struct
static struct tasklet_struct ex2_tasklet;

// Tasklet handler function - reschedules itself!
static void ex2_tasklet_handler(unsigned long data)
{
    static int count = 0;
    pr_info("Ex2 Repetitive Tasklet: Handler execution #%d\n", ++count);

    // Reschedule myself for the next run
    // Check if module is unloading? Not easily done here, rely on kill.
    if (count < 10) { // Let's limit it for sanity, otherwise it runs forever
        tasklet_schedule(&ex2_tasklet);
    } else {
        pr_info("Ex2 Repetitive Tasklet: Reached limit, stopping rescheduling.\n");
    }
}

// Statically declare the tasklet (using DECLARE_TASKLET)
// We need tasklet_init here because the handler needs the address of the tasklet
// static DECLARE_TASKLET(ex2_tasklet, ex2_tasklet_handler, 0); // Can't do this easily

static int __init ex2_init(void)
{
    pr_info("Ex2 Module: Loading...\n");

    // Initialize the tasklet dynamically
    tasklet_init(&ex2_tasklet, ex2_tasklet_handler, 0); // Pass 0 as data

    // Schedule the first execution
    pr_info("Ex2 Module: Scheduling first tasklet run.\n");
    tasklet_schedule(&ex2_tasklet);

    return 0; // Success
}

static void __exit ex2_exit(void)
{
    pr_info("Ex2 Module: Exiting...\n");

    // CRITICAL: Kill the tasklet. This prevents it from rescheduling
    // after the module code is gone, and waits if it's running.
    tasklet_kill(&ex2_tasklet);
    pr_info("Ex2 Module: Repetitive tasklet killed.\n");
    pr_info("Ex2 Module: Unloaded.\n");
}

module_init(ex2_init);
module_exit(ex2_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver Jędrzejczyk");
MODULE_DESCRIPTION("Exercise 2: Automatically repetitive tasklet");
MODULE_VERSION("1.0");