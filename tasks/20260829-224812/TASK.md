# Implement traditional comparison operators for the Query Language

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: tql

Extracted from TASK(20260829-200218). The traditional operators are
`<`, `<=`, `>`, `>=`, `=`, `!=`, etc.

Despite them having a special meaning in Shell we still wanna support
them in case the language is inputed not from a Shell environment.

Requires implementing a more sophisticated lexer: TASK(20260827-141739)

## NOTE(20260830-064623)

Done in the scope of TASK(20260829-200218) without any
TASK(20260827-141739) yet. So tokenization sucks, but we will fix it
later.
