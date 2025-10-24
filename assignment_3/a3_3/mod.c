// a3_3/mod.c — ps-style comparison fields (kernel 6.8+ safe)
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched/signal.h>   // task_struct, for_each_process
#include <linux/sched.h>          // task_state_index(), task_state_to_char(), policy
#include <linux/rcupdate.h>
#include <linux/cred.h>           // task_uid()
#include <linux/uidgid.h>         // __kuid_val()

MODULE_LICENSE("GPL");
MODULE_AUTHOR("minhgiga");
MODULE_DESCRIPTION("a3_3: print ps-like fields for comparison");
MODULE_VERSION("1.0");

/* policy names similar to chrt/ps output */
static const char *policy_name(unsigned int pol)
{
	switch (pol) {
	case SCHED_NORMAL:   return "CFS";
	case SCHED_BATCH:    return "BATCH";
	case SCHED_IDLE:     return "IDLE";
	case SCHED_FIFO:     return "FIFO";
	case SCHED_RR:       return "RR";
#ifdef SCHED_DEADLINE
	case SCHED_DEADLINE: return "DL";
#endif
	default:             return "UNK";
	}
}

static int __init mod_init(void)
{
	struct task_struct *p;

	rcu_read_lock();
	for_each_process(p) {
		/* ps -el rough mapping:
		   S/STAT ≈ task_state_to_char(); PID; PPID; PRI ≈ p->prio;
		   NI ≈ p->static_prio - 120; USER ≈ uid; CMD ≈ comm. */
		const kuid_t uid = task_uid(p);
		unsigned int st = task_state_index(p);
		char stc = task_state_to_char(p);
		int ppid = p->real_parent ? p->real_parent->pid : 0;
		int pri = p->prio;
		int ni  = p->static_prio - 120;   // user-nice approx
		int pol = p->policy;
		int rt  = p->rt_priority;

		printk(KERN_INFO "[PS] uid=%5u pid=%5d ppid=%5d pri=%4d ni=%3d pol=%-3s rt=%3d state_idx=%u stat=%c cmd=%s\n",
		       __kuid_val(uid), p->pid, ppid, pri, ni, policy_name(pol), rt, st, stc, p->comm);
	}
	rcu_read_unlock();

	return 0;
}

static void __exit mod_exit(void)
{
	printk(KERN_INFO "a3_3: module exit\n");
}

module_init(mod_init);
module_exit(mod_exit);
