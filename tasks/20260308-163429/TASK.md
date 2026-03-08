# Tasks array should be a hash table

- STATUS: OPEN
- PRIORITY: 30

Use ht.h

I don't think we can get that much performance boost as of right
now. To build the list of tasks we still iterate tasks folders
linearly one by one.
