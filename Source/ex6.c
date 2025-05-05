#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h> // tasklet API
#include <linux/slab.h>      // Slab allocator (kmem_cache)
#include <linux/proc_fs.h>   // Proc filesystem
#include <linux/seq_file.h>  // seq_file API for proc
#include <linux/spinlock.h>  // Spinlocks
#include <linux/list.h>      // Kernel lists
#include <linux/jiffies.h>   // jiffies, HZ
#include <linux/timer.h>     // Kernel timers
#include <linux/printk.h>    // pr_info

#define PROC_FILENAME "tasklet_stats"

// Structure to hold tasklet info and list linkage
typedef struct {
    struct tasklet_struct tasklet;
    struct list_head list;
    unsigned long data; // Example data
    bool high_priority;
    bool executed;
} tasklet_entry_t;

// Slab cache for our tasklet entries
static struct kmem_cache *tasklet_cache = NULL;

// List to keep track of created tasklets (for cleanup/stats)
static LIST_HEAD(active_tasklets);
static DEFINE_SPINLOCK(tasklet_list_lock); // Protect the list

// Statistics counters
static atomic_t created_count = ATOMIC_INIT(0);
static atomic_t scheduled_count = ATOMIC_INIT(0);
static atomic_t executed_count = ATOMIC_INIT(0);
static atomic_t high_prio_count = ATOMIC_INIT(0);
static atomic_t pending_count = ATOMIC_INIT(0);

// Generic tasklet handler
static void generic_tasklet_handler(unsigned long data)
{
    // We need the container structure pointer
    tasklet_entry_t *entry = (tasklet_entry_t *)data; // We pass the entry address as data
    unsigned long flags;

    pr_info("Ex6 Tasklet Handler: Executing tasklet created with data: %lu (High Prio: %d)\n",
            entry->data, entry->high_priority);

    atomic_inc(&executed_count);
    atomic_dec(&pending_count);

    // Mark as executed within the entry itself (requires lock)
    spin_lock_irqsave(&tasklet_list_lock, flags);
    entry->executed = true;
    spin_unlock_irqrestore(&tasklet_list_lock, flags);
}

// Function to create and schedule a tasklet dynamically
static tasklet_entry_t *create_and_schedule_tasklet(unsigned long task_data, bool high_prio)
{
    tasklet_entry_t *entry;
    unsigned long flags;

    entry = kmem_cache_alloc(tasklet_cache, GFP_KERNEL);
    if (!entry) {
        pr_err("Ex6: Failed to allocate tasklet entry from slab cache.\n");
        return NULL;
    }

    // Initialize the entry fields
    entry->data = task_data;
    entry->high_priority = high_prio;
    entry->executed = false;
    // Initialize the tasklet itself, passing the address of the container as data
    tasklet_init(&entry->tasklet, generic_tasklet_handler, (unsigned long)entry);

    // Add to our tracking list
    spin_lock_irqsave(&tasklet_list_lock, flags);
    list_add_tail(&entry->list, &active_tasklets);
    spin_unlock_irqrestore(&tasklet_list_lock, flags);

    // Update stats
    atomic_inc(&created_count);
    if (high_prio) {
        atomic_inc(&high_prio_count);
    }

    // Schedule it
    atomic_inc(&scheduled_count);
    atomic_inc(&pending_count); // Increment pending count here
    if (high_prio) {
        tasklet_hi_schedule(&entry->tasklet);
    } else {
        tasklet_schedule(&entry->tasklet);
    }
    pr_info("Ex6: Scheduled tasklet (Data: %lu, High Prio: %d)\n", task_data, high_prio);

    return entry;
}

// --- Proc File Implementation ---

static int tasklet_stats_show(struct seq_file *m, void *v)
{
    unsigned long flags;
    int current_pending = 0;
    tasklet_entry_t *entry;

    // Recalculate pending based on list traversal (more accurate than counter alone)
    spin_lock_irqsave(&tasklet_list_lock, flags);
    list_for_each_entry(entry, &active_tasklets, list) {
        // Check tasklet state (TASKLET_STATE_SCHED bit)
        // This is a bit hacky/internal, simpler to rely on our executed flag
        // if (test_bit(TASKLET_STATE_SCHED, &entry->tasklet.state)) {
        //     current_pending++;
        // }
        if (!entry->executed && test_bit(TASKLET_STATE_SCHED, &entry->tasklet.state)) {
             current_pending++;
        }
    }
    spin_unlock_irqrestore(&tasklet_list_lock, flags);

    // Use seq_printf for output
    seq_printf(m, "--- Tasklet Statistics ---\n");
    seq_printf(m, "Created:       %d\n", atomic_read(&created_count));
    seq_printf(m, "Scheduled:     %d\n", atomic_read(&scheduled_count));
    seq_printf(m, "Executed:      %d\n", atomic_read(&executed_count));
    seq_printf(m, "High Priority: %d\n", atomic_read(&high_prio_count));
    //seq_printf(m, "Currently Pending (Counter): %d\n", atomic_read(&pending_count));
    seq_printf(m, "Currently Pending (Scan):    %d\n", current_pending);

    return 0;
}

// Boilerplate for single proc file read
static int tasklet_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, tasklet_stats_show, NULL);
}

// Use proc_ops in newer kernels
static const struct proc_ops tasklet_stats_fops = {
    .proc_open = tasklet_stats_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

// --- Module Init/Exit ---

static int __init ex6_init(void)
{
    struct proc_dir_entry *proc_entry;

    pr_info("Ex6 Module: Loading...\n");

    // Create slab cache
    tasklet_cache = kmem_cache_create("ex6_tasklet_cache", // Name
                                      sizeof(tasklet_entry_t), // Size of objects
                                      0, // Alignment (0 = default)
                                      SLAB_PANIC, // Flags (panic if alloc fails)
                                      NULL); // Constructor (none needed)
    if (!tasklet_cache) {
        pr_err("Ex6: Failed to create slab cache.\n");
        return -ENOMEM;
    }
    pr_info("Ex6: Slab cache created.\n");

    // Create /proc entry
    proc_entry = proc_create(PROC_FILENAME, 0444, NULL, &tasklet_stats_fops); // Read-only for all
    if (!proc_entry) {
        pr_err("Ex6: Failed to create /proc/%s entry.\n", PROC_FILENAME);
        kmem_cache_destroy(tasklet_cache); // Clean up cache
        return -ENOMEM;
    }
    pr_info("Ex6: /proc/%s entry created.\n", PROC_FILENAME);

    // Create some tasklets for testing
    create_and_schedule_tasklet(100, false); // Normal prio
    create_and_schedule_tasklet(200, true);  // High prio
    create_and_schedule_tasklet(300, false); // Normal prio

    pr_info("Ex6 Module: Loaded successfully.\n");
    return 0;
}

static void __exit ex6_exit(void)
{
    struct list_head *pos, *n;
    tasklet_entry_t *entry;
    unsigned long flags;

    pr_info("Ex6 Module: Exiting...\n");

    // Remove /proc entry first
    remove_proc_entry(PROC_FILENAME, NULL);
    pr_info("Ex6: /proc/%s entry removed.\n", PROC_FILENAME);

    // Kill and free all active tasklets
    pr_info("Ex6: Cleaning up tasklets...\n");
    spin_lock_irqsave(&tasklet_list_lock, flags);
    list_for_each_safe(pos, n, &active_tasklets) {
        entry = list_entry(pos, tasklet_entry_t, list);
        list_del(&entry->list); // Remove from list *before* killing

        pr_info("Ex6: Killing tasklet (Data: %lu)\n", entry->data);
        // Unlock before kill, as kill might sleep
        spin_unlock_irqrestore(&tasklet_list_lock, flags);
        tasklet_kill(&entry->tasklet);
        kmem_cache_free(tasklet_cache, entry);
        spin_lock_irqsave(&tasklet_list_lock, flags); // Relock for next iteration
    }
    spin_unlock_irqrestore(&tasklet_list_lock, flags);
    pr_info("Ex6: Tasklet cleanup complete.\n");


    // Destroy slab cache (must be done after all objects freed)
    if (tasklet_cache) {
        kmem_cache_destroy(tasklet_cache);
        pr_info("Ex6: Slab cache destroyed.\n");
    }

    pr_info("Ex6 Module: Unloaded.\n");
}

module_init(ex6_init);
module_exit(ex6_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oliver Jędrzejczyk");
MODULE_DESCRIPTION("Exercise 6: Tasklet stats in /proc using slab");
MODULE_VERSION("1.0");