# Priority becomes irrelevant when the task is closed (modification date is more important)

- STATUS: OPEN
- PRIORITY: 90

When you do `tatr ls -c` you get the list of closed tasks which is
still sorted by the priority. But as soon as the task is closed, the
priority is irrelevant. What's more important in case of a closed task
is when it was closed. Maybe we should sort the closed tasks by the
date of its last modification.

---

Furthermore sorting open tasks by modification date is also useful
sometimes. For example when you just created a bunch of new tasks. You
wanna see them on top before you prioritize them.

---

Specifically for the case of sorting newer tasks for later
prioritization we can actually sort by the id itself. Because the id
contains the date of creation.

Moved this to 20260410-052830.
