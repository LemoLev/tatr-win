# Memory management is a bit messy

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: release

It is currently fine as long as this program is a batch program. But
we have a task for potentially adding a Web interface
(20260316-214021) which means it may become a long running interactive
program where memory management becomes more important.

I should go through the source code and make memory management more
predictable (whatever that means).

---

The main concern is undisciplined reliance on String_Builder-s. It
would be nice to have context allocators like in Jai so to not think
about this.

---

Things to check:
- [x] `String_Builder`
- [x] `read_entire_file`
- [x] `read_entire_dir`
- [x] `str*dup`

---

The `*_run` functions should probably not be that disciplined about
memory because they are designed to interface the batch interface of
the program.
