# The comparison operators in Query Language are lame

- STATUS: OPEN
- PRIORITY: 100
- TAGS: release

## NOTE(20260829-200429)

lt, gt, lte, gte, etc.

They were implemented as a temporary placeholder.

We could alias them to their traditional forms `<`, `>`, `<=`,
etc. But those have special meaning in Shell and can't be used
unescaped. We can still provide the aliases in case of usage outside
of Shell-like environments.

## NOTE(20260829-200435)

- `below`
- `above`
- `not above`
- `not below`
- `equal`
- `not equal`

It was also suggested in the Discord to consider things like `at least`
