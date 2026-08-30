# The syntax of tags in the query language is confusing

- STATUS: OPEN
- PRIORITY: 100
- TAGS: tql

## NOTE(20260829-201427)

In the query language you prefix them with dot: `.bug`. But in such
commands as `tatr-new` when you pass them via the `-t` flag you don't
put any dots: `-t bug`. This is confusing.

## NOTE(20260829-201431)

Just allowing tags without dots like `bug and release` means we need
to introduce the notion of keyword which could not be used as a
keyword which greatly reduces the vocabulary of tags.

## NOTE(20260829-201434)

Maybe long prefix: `tag:bug and tag:release`. Much less convenient to
type.

## NOTE(20260829-201439)

Keywords as special characters don't work well in Shell environment
because there they also have special meaning: `bug && relase`.

## NOTE(20260829-201442)

For some reason I feel like just replacing `.` with `:`:
`:bug and :release`. Not sure how it helps with confusing but it just
feels better?

It reminds me [keywords from Clojure](https://clojuredocs.org/clojure.core/keyword).

## NOTE(20260829-201447)

Actually, I feel like after TASK(20260828-211350) it became potentially less
confusing? Because now commands that accept query language have these
distinct boundaries between CLI arguments and the query language
expression.

I think I'm gonna implement NOTE(20260829-201442) and just unschedule
this task from release until I have more info on how actually
confusing people find this.
