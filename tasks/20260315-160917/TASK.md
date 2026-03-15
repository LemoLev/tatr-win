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
