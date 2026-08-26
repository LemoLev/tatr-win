# There should be a command that reports all the skipped weird folders and files found in the tasks/ folder

- STATUS: OPEN
- PRIORITY: 10

What for?

---

For example, you may have a valid task folder which doesn't contain
TASK.md. They user may wanna know about it.

---

And we want it to be a separate command because it is very common to
have such folders while using git because git doesn't track
folders. So you may accidentally create a task. Then discard TASK.md
leaving behind an empty task folder.
