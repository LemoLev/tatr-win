# Memory management is a bit messy

- STATUS: OPEN
- PRIORITY: 100
- TAGS: release,scope

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

Things to check
- [ ] `String_Builder`
- [ ] `read_entire_file`
- [ ] `read_entire_dir`
- [ ] `str*dup`
