# The syntax of tags in the query language is confusing

- STATUS: OPEN
- PRIORITY: 100
- TAGS: release

In the query language you prefix them with dot: `.bug`. But in such
commands as `tatr-new` when you pass them via the `-t` flag you don't
put any dots: `-t bug`. This is confusing.
