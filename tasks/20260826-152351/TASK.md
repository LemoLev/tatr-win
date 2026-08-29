# Stress-test the markdown parser

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: release

I usually don't put anything unusual into TASK.md myself, but other
people might. So I need to test how it all works on random junk.

---

As a result of working on this issue md parsing completely got rid of
error reporting. It is now similar to how we load songs in
sowon2. Invalid task is just a task that is still incorporated in the
results, but has a screaming appearance.

We may even consider doing just this instead of introducing separate
command for reporting weird task folders 20260308-171346.

## Problems to fix in this issue

### [FIXED] If `TAGS` is missing and the body starts with a list item, the parser thinks we provided an invalid property.

```
# title
- STATUS: OPEN
- PRIORITY: 69

- foo
- bar
- baz
```

It thinks `foo` is another property and fails. Not sure what to do about this.

### [FIXED] Properties can be defined only in a specific order.

Huge oversight. Should allow properties in any order and also
duplicated properties. In case of duplicates take the value of the
last one.

Unsupported properties should be probably just ignored.

### [MOVED] Invalid status value just silently does nothing and hides the task

I think this should be addressed by 20260826-200847
