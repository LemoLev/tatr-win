# Priority becomes irrelevant when the task is closed

- STATUS: OPEN
- PRIORITY: 100

When you do `tatr ls -c` you get the list of closed tasks which is
still sorted by the priority. But as soon as the task is closed, the
priority is irrelevant. What's more important in case of a closed task
is when it was closed. Maybe we should sort the closed tasks by the
date of its last modification.
