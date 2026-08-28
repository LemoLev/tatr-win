# Stress-test the markdown parser

- STATUS: OPEN
- PRIORITY: 100
- TAGS: release

I usually don't put anything unusual into TASK.md myself, but other
people might. So I need to test how it all works on random junk.

## Problems to fix in this issue

### If `TAGS` is missing and the body starts with a list item, the parser thinks we provided an invalid property.

```
# title
- STATUS: OPEN
- PRIORITY: 69

- foo
- bar
- baz
```

It thinks `foo` is another property and fails. Not sure what to do about this.

### Properties can be defined only in a specific order.

Huge oversight. Should allow properties in any order and also
duplicated properties. In case of duplicates take the value of the
last one.

Unsupported properties should be probably just ignored.
