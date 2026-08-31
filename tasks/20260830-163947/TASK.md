# TQL supports less tags than the spec allows to define

- STATUS: CLOSED
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

## NOTE(20260830-173544)

Fork from NOTE(20260830-170057)

Actually, what if the syntax for `OP_TAG` is not `:tag` but `{tag}`?

`tatr ls {bug} and {release}`

It's a bit annoying that you have to provided the ending symbol just to refer to a tag though. Before you only needed `:` at the start.

## NOTE(20260830-170102)

Another solution is to change the spec and just limit what kind of tags could be used in there.

It gets a bit messy when the unicode starts to get involved.

I feel like just forbidding whitespaces in the names of the tags solves majority of the problems.

If we are going that route the tool must validate tag names everywhere.

If would've been simplier if the whitespaces WERE the separators. But I guess it's too late for that now. We need to maintain backward compatibility with existing `tasks/` folders.

Though we could just say that commas are treated as whitespaces.

None of that solves the problem where in `not [:bug or :ui]` the last tag captures the bracket `ui]` because according to the spec `]` can be part of the tag. Applying NOTE(20260830-173544) turns it into `not [{bug} or {ui}]` which I think is a bit noisy.

Another idea is `not [:bug: or :ui:]`. Requires escaping `:` since it could be in a tag.

But you know what cannot be a tag? Comma! `not [,bug, or ,ui,]`. Nah, too goofy...

We could go even stricter and just allow only alphanumeric symbols in the tags.

But we already have a test with utf8.

But I don't use any utf8 in the wild.

It feels like tags must have a very strict limitation because they are used in the query language. So they must be in harmony with each other.

Also, it's easier to relax the limit later if needed.

## NOTE(20260831-045755)

I think this problem occured because I overcomplicated TQL. I cut down on all the aliases and updated the spec to treat commas as whitespace and now only tags with square brackets cannot be queries using `tatr`. Which is I think a reasonable limitation for now which I documented in the README.
