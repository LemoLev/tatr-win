# The way `tatr-ls` constructs queries is confusing

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: release

You can do things like

```console
$ tatr ls .bug -id and -a priority gte 100
```

`-id` and `-a` are not part of the query. The query is actually `.bug
and priority gte 100`. The fact that you can interleave query and the
flags like this is extremely confusing and should be either changed or
properly documented.

Same will go for any future commands that accept queries (see
20260828-211200).

---

I feel like `tatr-new` has a similar problem but with titles. We need
to double check on that.
