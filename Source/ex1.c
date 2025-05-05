#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h> // tasklet API
#include <linux/printk.h>    // pr_info

// Tasklet handler function
static void ex1_tasklet_handler(unsigned long data)
{
    pr_info("Ex1 Tasklet: Handler executing with data: %lu\n", data);
}

// Statically declare a tasklet, initially disabled
static DECLARE_TASKLET_DISABLED(ex1_tasklet, ex1_tasklet_handler, 123);

static int __init ex1_init(void)
{
    pr_info("Ex1 Module: Loading...\n");

    // Schedule the disabled tasklet. It shouldn't run yet.
    tasklet_schedule(&ex1_tasklet);
    pr_info("Ex1 Module: Tasklet scheduled (but disabled).\n");

    // Check if still disabled (note: no direct API to check, we infer)
    // We assume it didn't run yet. Let's enable it.
    pr_info("Ex1 Module: Enabling tasklet...\n");
    tasklet_enable(&ex1_tasklet);
    pr_info("Ex1 Module: Tasklet enabled. Should run soon.\n");
    // At this point, the previously scheduled tasklet should execute.

    // Schedule it again, it should run now as it's enabled
    tasklet_schedule(&ex1_tasklet);
    pr_info("Ex1 Module: Tasklet scheduled again (now enabled).\n");

    // Disable it again (waits if running - handler is fast anyway)
    pr_info("Ex1 Module: Disabling tasklet (sync)...\n");
    tasklet_disable(&ex1_tasklet);
    pr_info("Ex1 Module: Tasklet disabled (sync).\n");

    // Try scheduling while disabled - should have no effect until enabled
    tasklet_schedule(&ex1_tasklet);
    pr_info("Ex1 Module: Tasklet scheduled while disabled (no effect yet).\n");

    // Enable it again - the schedule above should now take effect
    pr_info("Ex1 Module: Enabling tasklet again...\n");
    tasklet_enable(&ex1_tasklet);
    pr_info("Ex1 Module: Tasklet enabled again. Should run soon.\n");

    // Schedule one more time
    tasklet_schedule(&ex1_tasklet);
    pr_info("Ex1 Module: Tasklet scheduled one last time.\n");

    // Disable without waiting (nosync)
    // Difference isn't obvious here as handler is quick and not running.
    pr_info("Ex1 Module: Disabling tasklet (nosync)...\n");
    tasklet_disable_nosync(&ex1_tasklet);
    pr_info("Ex1 Module: Tasklet disabled (nosync).\n");

    // Re-enable for cleanup, just in case
    tasklet_enable(&ex1_tasklet);


    return 0; // Success
}

static void __exit ex1_exit(void)
{
    pr_info("Ex1 Module: Exiting...\n");

    // Ensure the tasklet is removed from the queue and won't run after unload
    // Waits if the tasklet is currently running.
    tasklet_kill(&ex1_tasklet);
    pr_info("Ex1 Module: Tasklet killed.\n");
    pr_info("Ex1 Module: Unloaded.\n");
}

module_init(ex1_init);
module_exit(ex1_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver Jędrzejczyk");
MODULE_DESCRIPTION("Exercise 1: Tasklet enable/disable");
MODULE_VERSION("1.0");