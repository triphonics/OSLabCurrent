#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h> // work queue API
#include <linux/printk.h>    // pr_info
#include <linux/jiffies.h>   // HZ
#include <linux/err.h>       // IS_ERR, PTR_ERR
#include <linux/slab.h>      // kmalloc/kfree (if needed for data)

// Need a pointer for the dynamically allocated workqueue
static struct workqueue_struct *my_unbound_wq = NULL;

// Handlers are the same as Listing 2 / Exercise 5
static void normal_work_handler(struct work_struct *work)
{
    pr_info("Ex8 Normal Work Handler (Unbound WQ): Hi! I'm handler of normal work!\n");
}

static void delayed_work_handler(struct work_struct *work)
{
    pr_info("Ex8 Delayed Work Handler (Unbound WQ): Hi! I'm handler of delayed work!\n");
}

// Declare work items statically
static DECLARE_WORK(normal_work, normal_work_handler);
static DECLARE_DELAYED_WORK(delayed_work, delayed_work_handler);

static int __init ex8_init(void)
{
    pr_info("Ex8 Module (Alloc Unbound WQ): Loading...\n");

    // Allocate an unbound workqueue
    // Name format string, flags, max_active (0=unlimited for unbound)
    my_unbound_wq = alloc_workqueue("ex8_unbound_wq", WQ_UNBOUND, 0);
    if (!my_unbound_wq) {
        pr_err("Ex8: Failed to allocate unbound workqueue!\n");
        return -ENOMEM;
    }
    pr_info("Ex8: Allocated unbound workqueue 'ex8_unbound_wq'.\n");


    // Schedule normal work on our unbound queue
    if (!queue_work(my_unbound_wq, &normal_work)) {
        pr_info("Ex8: The normal work was already queued!\n");
    } else {
        pr_info("Ex8: Normal work scheduled on unbound queue.\n");
    }

    // Schedule delayed work on our unbound queue (delay 3 seconds)
    if (!queue_delayed_work(my_unbound_wq, &delayed_work, 3 * HZ)) {
        pr_info("Ex8: The delayed work was already queued!\n");
    } else {
         pr_info("Ex8: Delayed work scheduled on unbound queue (3 sec delay).\n");
    }

    return 0; // Success
}

static void __exit ex8_exit(void)
{
    pr_info("Ex8 Module (Alloc Unbound WQ): Exiting...\n");

    // Check if queue was actually created before trying to use/destroy it
    if (my_unbound_wq) {
        // Cancel the work items. These need to be cancelled BEFORE destroying the queue.
        pr_info("Ex8: Cancelling work items...\n");

        // Note: cancel_work_sync works on the work item, not the queue.
        // It will wait even if the handler is running on a worker from our queue.
        if (cancel_work_sync(&normal_work)) {
            pr_info("Ex8: The normal work has not been done yet! Cancelled.\n");
        } else {
             pr_info("Ex8: Normal work was not pending or already finished.\n");
        }

        if (cancel_delayed_work_sync(&delayed_work)) {
            pr_info("Ex8: The delayed work has not been done yet! Cancelled.\n");
        } else {
            pr_info("Ex8: Delayed work was not pending or already finished.\n");
        }

        // Now destroy the workqueue. This waits for workers to finish
        // any *currently running* handlers but doesn't wait for pending work.
        // That's why we cancel/flush first.
        pr_info("Ex8: Destroying unbound workqueue...\n");
        destroy_workqueue(my_unbound_wq);
        my_unbound_wq = NULL; // Avoid double free/use after free
        pr_info("Ex8: Unbound workqueue destroyed.\n");
    }


    pr_info("Ex8 Module (Alloc Unbound WQ): Unloaded.\n");
}

module_init(ex8_init);
module_exit(ex8_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver Jędrzejczyk");
MODULE_DESCRIPTION("Exercise 8: Listing 2 using alloc_workqueue (unbound)");
MODULE_VERSION("1.0");