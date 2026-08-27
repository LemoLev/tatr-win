# Stress-test the markdown parser

- STATUS: OPEN
- PRIORITY: 100
- TAGS: release

I usually don't put anything unusual into TASK.md myself, but other
people can. So I need to test how it all works on random junk.

---

I guess what worries me the most is inability of our parser to parse things like:

```
# title
- STATUS: OPEN
- PRIORITY: 69
- TAGS: scope

- foo
- bar
- baz
```

It thinks `foo` is another property and fails. Not sure what to do about this.
