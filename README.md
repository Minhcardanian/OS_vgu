# OS_vgu — Linux Kernel Module Exercises

This repository contains a set of Linux kernel modules developed as part of the Operating Systems coursework at Vietnamese-German University (VGU).  
All modules were built and tested on Ubuntu Server 24.04 LTS (kernel 6.8.0-85-generic) running inside a VirtualBox environment.

---

## Repository Structure


---

## 1. simple — Basic Kernel Module

### Description
A minimal kernel module demonstrating the structure of initialization and cleanup routines.

### Source
`simple/simple.c`

### Build and Load
```bash
make
sudo insmod simple.ko
dmesg | tail -n 5
sudo rmmod simple

---
## 2. proc_jiffies — Display System Jiffies
Description

Creates a /proc/jiffies entry that shows the current jiffies counter, a low-level timing mechanism in Linux.
Build and Run:
cd proc_jiffies
make
sudo insmod proc_jiffies.ko
cat /proc/jiffies
sudo rmmod proc_jiffies
--- 
## 3. proc_seconds — Track Module Lifetime
Description

This module registers /proc/seconds and prints how long the module has been loaded.
cd proc_seconds
make
sudo insmod proc_seconds.ko
sleep 5
cat /proc/seconds
sudo rmmod proc_seconds
---
BUILD ENV
| Component      | Version               |
| -------------- | --------------------- |
| Ubuntu         | 24.04.3 LTS (Server)  |
| Kernel         | 6.8.0-85-generic      |
| GCC            | 13.3.0                |
| Make           | 4.3                   |
| Virtualization | Oracle VirtualBox 7.x |


