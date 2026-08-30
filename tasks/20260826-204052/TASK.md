# Extended HUID

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: release

## NOTE(20260829-233518)

`YYYMMDD-HHMMSS[-<random-suffix>]`

Basically allow any suffix at the end after a dash. This should
address potential collisions when working in a team.

Just add support for that on the reading level. We may add support for
that on creation level later -- basically allow configuring the suffix
per user machine so the team member can agree on the collision
resolution scheme among themselves, maybe create a separate task for
that for after the release.

## NOTE(20260829-233521)

Another idea is to allow any folder within `tasks/` to be considered a
task folder. It's name is the ID.

One downside I see is what if we ever need special folders that are
not tasks in there.

The NOTE(20260829-233518) approach is basically that but mandates
prefixing task folders with `YYYMMDD-HHMMSS` to indicate that they are
indeed task folders.

## NOTE(20260830-000502)

Settled on `/[0-9]{8}-[0-9]{6}(-[a-zA-Z0-9\\-]*)?/`. Allowing random
junk makes them unreadable. And they meant to be readable.

An example of a task with Extended ID is TASK(20260830-000838-rexim)
