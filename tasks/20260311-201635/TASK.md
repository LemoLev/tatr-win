# `tatr ls` query language does not work well with UTF-8

- STATUS: CLOSED
- PRIORITY: 100

We need to test that at least.

---

The main problem is error reporting. The cursor's offset just
overshoots on longer characters because we compute distances in bytes
instead of characters.
