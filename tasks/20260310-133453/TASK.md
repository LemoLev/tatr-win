# Query language for `tatr ls`

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: stream

Maybe instead of implementing tons of mutually exclusive flags
(See 20260304-174730, 20260304-175116), I should just develop a query
language.

Something akin to Bex:

- `tag(editor)` - contains tag `editor`
- `and(tag(editor), tag(bug))` - contains tag `editor` and `bug`
- `or(tag(editor), tag(bug))` - contains tag `editor` or `bug`
- ...

Though, Bex syntax is a bit vebose for this. I should come up with
something more fitting.

- `.editor` - contains tag `editor`
- `.editor and .bug` - contains tags `editor` and `bug`
- `.editor or .bug` - contains tags `editor` or `bug`
- ..

This kind of style requires thinking about presedence. But maybe it's
worth it.

---

Done!
