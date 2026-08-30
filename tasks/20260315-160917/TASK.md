# When you have several TASK.md opened, it's unclear which issues they belong to

- STATUS: OPEN
- PRIORITY: 100
- TAGS: bug

This is more of a bug of the spec of the task database. All the info
about the issue is store in a file named "TASK.md". In a text editor
"TASK.md" is what's gonna be displayed in the title of a tab or a
window when the file is opened. Which gives zero info about what that
issues is about when you have several of them opened.

One solution I see is to extend the name of "TASK.md" with an optional
suffix "TASK[-few-word-from-the-title].md". But that opens up another
problem which is now a single Task may have several TASK.md-s.

---

Another solution is to extend the folder name
`<HUID>[-few-words-from-the-title]`. But that correspondingly means
you can have several tasks with the same HUID. But solution to that
could be to make the "few words" suffix part of the Task ID. Which
makes the Task IDs rather huge on the other hand.

Besides it's unclear how do you automatically form such IDs. It must
be up to the tool to do that. We getting into the territory of
language models which is a huge complexity for the problem I'm trying
to solve.

---

The `<HUID>[-few-words-from-the-title]` idea is implemented with
Extended HUID. See TASK(20260826-204052). But it's intended as a
mechanism for resolving collisions when working in a team. So I'm not
sure how much it is applicable here.
