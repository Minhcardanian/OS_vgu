// pid_mod.c — writes a PID to /proc/pid; reads back command, pid, state index
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <linux/sched.h>        // task_state_index(), task_state_to_char()
#include <linux/rcupdate.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("minhgiga");
MODULE_DESCRIPTION("Project 2: /proc/pid → show task info");
MODULE_VERSION("1.0");

static struct proc_dir_entry *proc_entry;
static int stored_pid = -1;
static char outbuf[128];

static ssize_t pid_read(struct file *f, char __user *ubuf, size_t len, loff_t *ppos)
{
	if (stored_pid < 0)
		return 0;

	rcu_read_lock();
	{
		struct task_struct *t = pid_task(find_vpid(stored_pid), PIDTYPE_PID);
		int n = 0;
		if (t) {
			unsigned int st = task_state_index(t);
			n = scnprintf(outbuf, sizeof(outbuf),
			              "command = [%s] pid = [%d] state = [%u]\n",
			              t->comm, t->pid, st);
		}
		rcu_read_unlock();
		if (!n) return 0;
		return simple_read_from_buffer(ubuf, len, ppos, outbuf, n);
	}
}

static ssize_t pid_write(struct file *f, const char __user *ubuf, size_t len, loff_t *ppos)
{
	char buf[32];
	size_t n = min(len, sizeof(buf) - 1);
	if (copy_from_user(buf, ubuf, n)) return -EFAULT;
	buf[n] = 0;

	if (kstrtoint(buf, 10, &stored_pid) != 0)
		return -EINVAL;

	return len;
}

static const struct proc_ops ops = {
	.proc_read  = pid_read,
	.proc_write = pid_write,
};

static int __init pid_init(void)
{
	proc_entry = proc_create("pid", 0666, NULL, &ops);
	if (!proc_entry) return -ENOMEM;
	pr_info("pid_mod loaded\n");
	return 0;
}

static void __exit pid_exit(void)
{
	if (proc_entry) remove_proc_entry("pid", NULL);
	pr_info("pid_mod unloaded\n");
}

module_init(pid_init);
module_exit(pid_exit);
