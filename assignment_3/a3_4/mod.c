// a3_4/mod.c — list threads per process (kernel 6.8+ safe)
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched/signal.h>   // task_struct, for_each_process, for_each_thread
#include <linux/sched.h>          // task_state_index(), task_state_to_char()
#include <linux/rcupdate.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("minhgiga");
MODULE_DESCRIPTION("a3_4: enumerate threads (LWPs) for each process");
MODULE_VERSION("1.0");

static int __init mod_init(void)
{
    struct task_struct *p, *t;

    rcu_read_lock();

    for_each_process(p) {
        unsigned int pst = task_state_index(p);
        char psc = task_state_to_char(p);

        /* Print the process (thread-group leader) */
        printk(KERN_INFO "[PROC] comm=%s pid=%d tgid=%d state_idx=%u stat=%c\n",
               p->comm, p->pid, p->tgid, pst, psc);

        /* Print each thread (includes the leader itself as first iteration) */
        for_each_thread(p, t) {
            unsigned int tst = task_state_index(t);
            char tsc = task_state_to_char(t);
            printk(KERN_INFO "  [THR]  tcomm=%s tpid=%d tgid=%d state_idx=%u stat=%c\n",
                   t->comm, t->pid, t->tgid, tst, tsc);
        }
    }

    rcu_read_unlock();
    return 0;
}

static void __exit mod_exit(void)
{
    printk(KERN_INFO "a3_4: module exit\n");
}

module_init(mod_init);
module_exit(mod_exit);
