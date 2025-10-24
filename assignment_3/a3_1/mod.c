// a3_1/mod.c — linear task listing with for_each_process (6.8+ safe)
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched/signal.h> // task_struct, for_each_process
#include <linux/sched.h>        // task_state_index(), task_state_to_char()
#include <linux/rcupdate.h>     // rcu_read_lock/unlock

MODULE_LICENSE("GPL");
MODULE_AUTHOR("minhgiga");
MODULE_DESCRIPTION("a3_1: linear task list using for_each_process");
MODULE_VERSION("1.1");

static int __init mod_init(void) {
    struct task_struct *p;

    rcu_read_lock();
    for_each_process(p) {
        unsigned int st = task_state_index(p);    // stable API
        char stc = task_state_to_char(p);         // like ps STAT first letter
        printk(KERN_INFO "[LIN] comm=%s pid=%d state_idx=%u stat=%c\n",
               p->comm, p->pid, st, stc);
    }
    rcu_read_unlock();

    return 0;
}

static void __exit mod_exit(void) {
    printk(KERN_INFO "a3_1: module exit\n");
}

module_init(mod_init);
module_exit(mod_exit);
