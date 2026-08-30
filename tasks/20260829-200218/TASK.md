# The comparison operators in Query Language are lame

- STATUS: OPEN
- PRIORITY: 100
- TAGS: release,tql

lt, gt, lte, gte, etc.

They were implemented as a temporary placeholder.

## NOTE(20260829-200429)

We could alias them to their traditional forms `<`, `>`, `<=`,
etc. But those have special meaning in Shell and can't be used
unescaped. We can still provide the aliases in case of usage outside
of Shell-like environments.

---

Traditional aliases requires more sophisticated lexing though which
requires implementing TASK(20260827-141739).

## NOTE(20260829-200435)

- `below`
- `above`
- `not above`
- `not below`
- `equal`
- `not equal`

It was also suggested in the Discord to consider things like `at least`

## NOTE(20260829-221229)

The ability to type `not above` implies the ability to type `not lt`,
`not gt`, etc. which makes it even more lame. Maybe we should just
remove the weird lame comparison operators as early as possible.

I didn't use them for too long to develop any important reflexes, so I
don't need to maintain backward compat in here.

## NOTE(20260830-035331)

I don't like that the most common operation with priority is `priority not below 100`

Maybe as mentioned in NOTE(20260829-200435) consider using `at least` and `at most`?

## NOTE(20260830-044156)

Another set I came up with

- `{`
- `{=`
- `}`
- `}=`
- `=`
- `!=`

`tatr ls not [:bug and priority }= 10]` looks like ass though

## NOTE(20260830-050449)

- `v`
- `v=`
- `^`
- `^=`
- `=`
- `!=`

`tatr ls not [:bug and priority ^= 10]`

## NOTE(20260830-050948)

The ХУЯ set

- `-?`
- `-?=`
- `+?`
- `+?=`
- `=`
- `!=`

`tatr ls not [:bug and priority +?= 10]`

## NOTE(20260830-064508)

At the end I settled on

- `lt`
- `lt=`
- `gt`
- `gt=`
- `=`
- `!=`

I can't improve this any further.
