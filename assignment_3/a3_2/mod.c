// a3_2/mod.c — DFS traverse the process tree from init_task
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched/signal.h> // task_struct
#include <linux/sched.h>        // task_state_index, task_state_to_char
#include <linux/rcupdate.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("minhgiga");
MODULE_DESCRIPTION("a3_2: DFS process tree from init_task");
MODULE_VERSION("1.0");

static void dfs(struct task_struct *t, int depth)
{
    struct list_head *lh;
    struct task_struct *child;

    /* print one line for this task */
    printk(KERN_INFO "[DFS]%*scomm=%s pid=%d state_idx=%u stat=%c\n",
           depth*2, "", t->comm, t->pid,
           task_state_index(t), task_state_to_char(t));

    /* recurse to children */
    list_for_each(lh, &t->children) {
        child = list_entry(lh, struct task_struct, sibling);
        dfs(child, depth + 1);
    }
}

static int __init mod_init(void)
{
    rcu_read_lock();
    dfs(&init_task, 0);
    rcu_read_unlock();
    return 0;
}

static void __exit mod_exit(void)
{
    printk(KERN_INFO "a3_2: module exit\n");
}

module_init(mod_init);
module_exit(mod_exit);
