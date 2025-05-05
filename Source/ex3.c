#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>    // kthread_run, kthread_stop
#include <linux/list.h>       // Linux kernel lists
#include <linux/slab.h>       // kmalloc, kfree
#include <linux/spinlock.h>   // spin locks
#include <linux/wait.h>       // wait queues
#include <linux/sched.h>      // TASK_INTERRUPTIBLE
#include <linux/delay.h>      // msleep
#include <linux/printk.h>     // pr_info

// Structure for our custom work item
typedef struct {
    struct list_head list; // Link for the list
    void (*func)(void *);  // Function to execute
    void *data;            // Data for the function
} simple_work_t;

// Global variables for our simple queue
static LIST_HEAD(work_list);             // Head of the work list
static DEFINE_SPINLOCK(list_lock);       // Spinlock to protect the list
static wait_queue_head_t worker_waitqueue; // Wait queue for the worker thread
static struct task_struct *worker_thread = NULL; // Worker thread task struct

// The actual function doing the "work"
static void simple_do_work(void *data)
{
    int id = *(int *)data;
    pr_info("SimpleWQ: Doing work with data ID: %d\n", id);
    kfree(data); // Free the data allocated in submit_work
}

// Worker thread function
static int worker_thread_fn(void *data)
{
    simple_work_t *work_item;
    unsigned long flags;

    pr_info("SimpleWQ: Worker thread started.\n");

    while (!kthread_should_stop()) {
        // Wait until there's work or we should stop
        wait_event_interruptible(worker_waitqueue,
                                 !list_empty(&work_list) || kthread_should_stop());

        if (kthread_should_stop()) {
            pr_info("SimpleWQ: Worker thread received stop signal.\n");
            break;
        }

        // Process all items currently in the list
        while (1) {
            spin_lock_irqsave(&list_lock, flags);
            if (list_empty(&work_list)) {
                spin_unlock_irqrestore(&list_lock, flags);
                break; // No more work for now
            }
            // Get the first work item
            work_item = list_first_entry(&work_list, simple_work_t, list);
            list_del(&work_item->list); // Remove from list
            spin_unlock_irqrestore(&list_lock, flags);

            // Execute the work function
            pr_info("SimpleWQ: Worker executing function %pS\n", work_item->func);
            work_item->func(work_item->data);

            // Free the work item structure
            kfree(work_item);
            work_item = NULL; // Good practice
        }
        // Yield just in case, though wait_event usually handles scheduling
        // schedule(); // Not strictly necessary here
    }

    pr_info("SimpleWQ: Worker thread stopping.\n");
    return 0;
}

// Function to submit work to our simple queue
static int submit_work(void (*func)(void *), int id)
{
    simple_work_t *new_work;
    int *data_copy;
    unsigned long flags;

    // Allocate memory for the work item
    new_work = kmalloc(sizeof(simple_work_t), GFP_KERNEL);
    if (!new_work) {
        pr_err("SimpleWQ: Failed to allocate memory for work item\n");
        return -ENOMEM;
    }

    // Allocate memory for the data (just an int here)
    data_copy = kmalloc(sizeof(int), GFP_KERNEL);
    if (!data_copy) {
        kfree(new_work);
        pr_err("SimpleWQ: Failed to allocate memory for work data\n");
        return -ENOMEM;
    }
    *data_copy = id;

    // Initialize the work item
    INIT_LIST_HEAD(&new_work->list);
    new_work->func = func;
    new_work->data = data_copy;

    // Add to the list (protected by spinlock)
    spin_lock_irqsave(&list_lock, flags);
    list_add_tail(&new_work->list, &work_list);
    spin_unlock_irqrestore(&list_lock, flags);

    // Wake up the worker thread
    wake_up(&worker_waitqueue);

    pr_info("SimpleWQ: Submitted work with ID %d\n", id);
    return 0;
}

static int __init ex3_init(void)
{
    pr_info("SimpleWQ Module: Loading...\n");

    // Initialize the wait queue
    init_waitqueue_head(&worker_waitqueue);

    // Create and start the worker thread
    worker_thread = kthread_run(worker_thread_fn, NULL, "simple_worker");
    if (IS_ERR(worker_thread)) {
        pr_err("SimpleWQ: Failed to create worker thread (%ld)\n", PTR_ERR(worker_thread));
        return PTR_ERR(worker_thread);
    }

    // Submit some work items
    submit_work(simple_do_work, 1);
    submit_work(simple_do_work, 2);
    msleep(10); // Give worker time to process first batch
    submit_work(simple_do_work, 3);

    pr_info("SimpleWQ Module: Loaded successfully.\n");
    return 0;
}

static void __exit ex3_exit(void)
{
    unsigned long flags;
    struct list_head *pos, *n;
    simple_work_t *work_item;

    pr_info("SimpleWQ Module: Exiting...\n");

    // Stop the worker thread
    if (worker_thread) {
        pr_info("SimpleWQ: Stopping worker thread...\n");
        kthread_stop(worker_thread);
        worker_thread = NULL;
        pr_info("SimpleWQ: Worker thread stopped.\n");
    }

    // Cleanup any remaining work items in the list (important!)
    pr_info("SimpleWQ: Cleaning up remaining work items...\n");
    spin_lock_irqsave(&list_lock, flags);
    list_for_each_safe(pos, n, &work_list) {
        work_item = list_entry(pos, simple_work_t, list);
        list_del(&work_item->list);
        pr_info("SimpleWQ: Cleaning work with data ID %d\n", *(int *)work_item->data);
        kfree(work_item->data);
        kfree(work_item);
    }
    spin_unlock_irqrestore(&list_lock, flags);
    pr_info("SimpleWQ: Cleanup complete.\n");

    pr_info("SimpleWQ Module: Unloaded.\n");
}

module_init(ex3_init);
module_exit(ex3_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver Jędrzejczyk");
MODULE_DESCRIPTION("Exercise 3: Simplified work queue implementation");
MODULE_VERSION("1.0");