# Tasks Database

This is an improvised Tasks Tracker, because I do need something more powerful than just plane TODOs in the Source Code, yet I don't want to install a full blown Issue Tracker System.

## How does it work

Each folder is a Task. The folder name is the Task ID. The Task ID format is `[0-9]{8}-[0-9]{6}`. Just grab the current date and time and use it as the Task ID. Use UTC timezone so the current timezone is irrelevant. If your Task ID collides with an existing one, just wait one second and try again. ;)

Inside of the Task folder there is one mandatory file `TASK.md` which is markdown file describing the Task. The folder may contain other files as attachments to the task. `TASK.md` should link to the attachments as necessary. Try to keep the size of the attachments small, since they are going to be committed to the git repo. Use [ffmpeg](https://ffmpeg.org/) to reencode any screencast to reduce their size as necessary.

The format of `TASK.md`:

```markdown
# <title-of-the-task>

- STATUS: <OPEN|CLOSED>
- PRIORITY: <number>
[- TAGS: <comma-separated-list-of-tags>]

[description-of-the-task]
```

As you work on the Task feel free append any discovered details about the Task to the description.

Use `git-blame` and `git-log` to learn about when, how and by whom any changes to the Task were made.

### Tag descriptions

There might be an optional `tasks/tags` file with the following format:

```
<tag-name> , <tag-description>
<tag-name> , <tag-description>
<tag-name> , <tag-description>
...
```

It serves as a documentation for each existing tag and the thirdparty tools may use it to display the tag descriptions.

## `tasks` utility

There is a simple tool for manipulating the Tasks database [tasks.c](./tasks.c). Just compile it and start using it:

```console
$ cc -o tasks tasks.c
$ ./tasks             # list all open tasks
$ ./tasks new "Title" # create a new task
$ ./tasks help        # print the list of commands
```

Don't expect too much from it though. Feel free to add missing functionality you need.
