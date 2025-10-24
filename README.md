# Assignment 2 & 3 — Requirements and Subtasks

## Assignment 2 — `/proc/pid` kernel module (task info by PID)

### Goal
Write a Linux kernel module that creates `/proc/pid`. When a PID is written to this file, subsequent reads must output the command name, PID value, and current state for that task. Example usage:



### Core behavior
- Implement a `/proc` entry with a **write** handler that accepts the PID as text, parses it to an integer, and stores it for later reads. Use `kstrtol()` to parse; manage memory as needed and copy from userspace with `copy_from_user()`.
- Implement a **read** handler that, given the stored PID, locates the corresponding `task_struct` and prints `(command, pid, state)` to the caller.

### Hints / APIs
- Resolve PID → task with `find_vpid(int pid)` then `pid_task(..., PIDTYPE_PID)` to obtain a `task_struct *`. Handle invalid PIDs gracefully by returning `0` bytes on read (EOF).
- Wire your `/proc` file with your custom `.read` and `.write` callbacks in the `proc_ops` table.

### Subtasks checklist
1. Create `/proc/pid` with custom `.write` and `.read`. Parse incoming PID with `kstrtol()`.
2. Resolve `task_struct` from PID using `find_vpid()` → `pid_task(..., PIDTYPE_PID)`.
3. On `/proc/pid` **read**, print `command`, `pid`, `state` for the stored task.
4. Handle errors: if PID is invalid or task not found, the read should return `0`.

---

## Assignment 3 — Enumerate tasks (linear & DFS)

### Goal
Write kernel modules that list all current tasks in Linux, first **linearly** and then via a **depth-first search (DFS)** of the process tree, printing each task’s command, state, and PID.

### Part 1 — Linear iteration
- Iterate over all tasks using the `for_each_process()` macro and print `(command, pid, state)` for each task.
- Do this in the module’s init function so output appears in the kernel log buffer. Compare the output to `ps -el` (differences are normal due to system activity and differing representations).

### Part 2 — DFS over the process tree
- Starting from `init_task`, traverse the **children** and **sibling** lists to visit the process tree in DFS order; print `(command, pid, state)` for each task.
- Use kernel list traversal macros to iterate `task->children` and grab each child with `list_entry`.
- Validate hierarchy (and threads) with `ps -eLf`, which shows LWPs/threads.

### Subtasks checklist
1. **Linear module**: use `for_each_process()`; print `comm`, `pid`, and state for every task; view in `dmesg`.
2. **Compare vs. ps**: check `dmesg` output against `ps -el`. Minor differences are expected.
3. **DFS module**: traverse `children`/`sibling` from `init_task` and print `(command, pid, state)` for each in DFS order.
4. **Validate threads**: confirm with `ps -eLf` since some items are threads that won’t show as ordinary processes.

---

### Notes
- All outputs should go to the **kernel log buffer**; use `sudo dmesg` or `sudo journalctl -k` to view results.
- Useful headers: `<linux/sched/signal.h>` (task traversal), `<linux/sched.h>` (state helpers), and `<linux/list.h>` (list iteration). On newer kernels, prefer `task_state_index()` / `task_state_to_char()` over directly reading `task_struct.state`.
