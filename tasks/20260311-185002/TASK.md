# Query "not (tagged or .bug and .test and .foo and .bar)" is not parsed properly

- STATUS: CLOSED
- PRIORITY: 100
- TAGS: bug

Introduced by 20260310-133453

Steps to reproduce:

```console
$ ./build/tatr ls -df -f "not (tagged or .bug and .test and .foo and .bar)"
not (tagged or .bug and .test and .foo and .bar)
                                               ^
ERROR: expected end of primary expression
$
```

Expected output is roughly:

```
OP_TAGGED
OP_TAG bug
OP_TAG test
OP_AND
OP_TAG foo
OP_AND
OP_TAG bar
OP_OR
OP_NOT
```
