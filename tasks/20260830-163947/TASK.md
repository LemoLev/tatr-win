# TQL supports less tags than the spec allows to define

- STATUS: OPEN
- PRIORITY: 100
- TAGS: tql,release

## NOTE(20260830-170057)

Maybe introduce some sort of `:'complex tag'` syntax.

Basically `'foo bar baz'` could be treated as a single tag. Though Shell tokenization gets in a way here.

We could use curly braces `:{complex tag}`.

But how do we escape `{` and `}` within the `{complex tag}`? We can just check the balance and allow `{complex {tag}}`. But what if I need something out of balance? `{complex } tag}`.

We can use `^` as an escape symbol (since Shell hijacks backslash): `{complex ^} tag}` and `{complex ^^ tag}`.

We should also add a section about tokenization to the README.

Shell tokenization may get in the way by turning `{foo     bar}` into `{foo bar}`.

## NOTE(20260830-170102)

Another solution is to change the spec and just limit what kind of tags could be used in there.

It gets a bit messy when the unicode starts to get involved.

## NOTE(20260830-173544)

Fork from NOTE(20260830-170057)

Actually, what if the syntax for `OP_TAG` is not `:tag` but `{tag}`?

`tatr ls {bug} and {release}`

It's a bit annoying that you have balance the curly braces now though.
