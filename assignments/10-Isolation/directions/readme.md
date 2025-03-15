# Linux User Namespace Lab Assignment (No Root Required)

## Overview
In this lab, you will explore Linux user namespaces, which are a fundamental building block of container technology. User namespaces are the only namespace type that can be manipulated without root privileges, making them perfect for student environments. You'll learn how user namespaces provide isolation and capabilities to unprivileged users.

## Learning Objectives
- Understand the concept of Linux user namespaces
- Learn how to create and manage user namespaces without root privileges
- Experience how UID/GID mapping works in practice
- Observe how capabilities function inside user namespaces

## Prerequisites
- Access to a Linux machine (Ubuntu 20.04+ or other modern distribution)
- Unprivileged user namespace support must be enabled
- Basic command-line familiarity

## Checking System Compatibility
Before starting the lab, check if your system supports unprivileged user namespaces:

```
cat /proc/sys/kernel/unprivileged_userns_clone
```

If this returns `1`, your system supports unprivileged user namespaces. If it returns `0`, you'll need to request your system administrator to enable this feature (or use a different system).

## Part 1: Exploring User Namespaces Basics

1. Examine your current user ID and groups:

```
id
```
Record this information for comparison later.

2. List your current namespaces:

```
ls -la /proc/self/ns/
```
Take note of the user namespace identifier.

EXAMPLE output (yours will be different):

```bash
ls -la /proc/self/ns/
total 0
dr-x--x--x 2 nobody nogroup 0 Mar  8 12:14 .
dr-xr-xr-x 9 nobody nogroup 0 Mar  8 12:14 ..
lrwxrwxrwx 1 nobody nogroup 0 Mar  8 12:14 cgroup -> 'cgroup:[4026531835]'
lrwxrwxrwx 1 nobody nogroup 0 Mar  8 12:14 ipc -> 'ipc:[4026531839]'
lrwxrwxrwx 1 nobody nogroup 0 Mar  8 12:14 mnt -> 'mnt:[4026531841]'
lrwxrwxrwx 1 nobody nogroup 0 Mar  8 12:14 net -> 'net:[4026531840]'
lrwxrwxrwx 1 nobody nogroup 0 Mar  8 12:14 pid -> 'pid:[4026531836]'
lrwxrwxrwx 1 nobody nogroup 0 Mar  8 12:14 pid_for_children -> 'pid:[4026531836]'
lrwxrwxrwx 1 nobody nogroup 0 Mar  8 12:14 time -> 'time:[4026531834]'
lrwxrwxrwx 1 nobody nogroup 0 Mar  8 12:14 time_for_children -> 'time:[4026531834]'
lrwxrwxrwx 1 nobody nogroup 0 Mar  8 12:14 user -> 'user:[4026532851]'
lrwxrwxrwx 1 nobody nogroup 0 Mar  8 12:14 uts -> 'uts:[4026531838]'
```

3. Create a new user namespace:

```
unshare --user bash
```

4. Inside this new bash shell (in the new user namespace), check your user and group IDs:

```
id
```

Notice how your UID and GID appear as 65534 (nobody).

5. Examine the namespace identifier to confirm you're in a different user namespace:

```
ls -la /proc/self/ns/user
```

Compare it with the value you noted earlier.

You might also try `whoami` and observe that it prints "nobody".

6. Check your new namespace's UID mapping:

```
cat /proc/self/uid_map
cat /proc/self/gid_map
```
Note that these files are likely empty, indicating no mappings are established.

7. Exit the namespace:

```
exit
```

## Part 2: Creating User Namespaces with UID/GID Mappings

For this part, we'll use a simple C program to create a user namespace with custom mappings. The C program is `userns_child.c` in the starter folder

Compile and run this program:

```
make
./userns_child
```

Observe the output and answer these questions:

- What is your UID in the new user namespace?
- What capabilities do you have in the user namespace?
- How do the UID mappings work?

## Part 3: Exploring Isolation with User Namespaces

Run `check_isolation.sh` in the starter folder:

```bash
chmod +x check_isolation.sh
./check_isolation.sh
```

Note the differences in the output before and after entering the user namespace.

## Part 4: Practical Applications - Running a Program with "Root" Privileges

1. Compile and use it to run commands as "root" (in the namespace):

```
make
./fake_root id
./fake_root bash -c "id && whoami && ls -la /proc/self/ns/"
```

Notice that inside the user namespace, your UID is 0 (root), but this is only within the namespace!

2. Try to create a file as "root" in your home directory, using your program:

```
./fake_root touch ~/root_test_file
ls -la ~/root_test_file
```

The file is created, but notice who owns it on the real system!

## Part 5: Exploring the Limitations

1. Try to perform privileged operations in your user namespace:

```
mkdir -p /tmp/test_mount
./fake_root bash -c "mount -t tmpfs none /tmp/test_mount"
```

This will likely fail with "Operation not permitted" because of missing capabilities or namespace configurations.

2. Try to access network namespaces (which usually require real root):

```
./fake_root ip link add dummy0 type dummy
```

Note and document what errors you encounter.

## Deliverables

Prepare a report in plain .txt or markdown .md containing:

1. A brief introduction to Linux user namespaces (in your own words)
2. Terminal outputs from each part of the lab
3. Answers to the following questions:
   - How do user namespaces provide the illusion of having root privileges?
   - What is the purpose of UID/GID mapping in user namespaces?
   - What limitations did you encounter when working with user namespaces?
   - How might user namespaces be used in container technology?
   - What security implications do user namespaces have?
   - Why are other namespace types typically not available to unprivileged users?

4. A conclusion section with your insights and any challenges you faced.

## Submission Instructions

1. Compile all your command outputs and answers into a single .txt or .md document.
2. Submit the document itself (not a git repo link) to the "Week 10 - Isolation" assignment in Blackboard.

## What to hand in and Grading Rubric

Grading Rubric
- 25 points: Completeness of terminal outputs
- 25 points: Quality of question answers

Total points achievable is 50.
