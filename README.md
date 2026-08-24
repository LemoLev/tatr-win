# Tasks Database

This is spec for an improvised Tasks system, because I needed something more powerful than just plane TODOs in the Source Code of my projects, yet I didn't want to install a full blown Issue Tracker System.

## How does it work

```
project/
+-...
+-tasks/
  +-tags
  +-20260824-215300
    +-TASK.md
    +-...
  +-...
```

Each project at the root has `tasks/` folder which contains sub-folders for each task.

Each task sub-folder is named with a Task ID. The Task ID format is `[0-9]{8}-[0-9]{6}`. Just grab the current date and time and use it as the Task ID. Use UTC timezone so the current timezone is irrelevant. If your Task ID collides with an existing one, just wait one second and try again. ;)

Inside of the task sub-folder there is one mandatory file `TASK.md` which is a markdown file describing the task. The folder may contain other files as attachments to the task. `TASK.md` should link to the attachments as necessary. Try to keep the size of the attachments small, since they are going to be committed to the git repo. Use [ffmpeg](https://ffmpeg.org/) to reencode any screencast to reduce their size as necessary.

The format of `TASK.md`:

```markdown
# <title-of-the-task>

- STATUS: <OPEN|CLOSED>
- PRIORITY: <number>
[- TAGS: <comma-separated-list-of-tags>]

[description-of-the-task]
```

As you work on the task feel free to append any discovered details about the task to the description.

Use `git-blame` and `git-log` to learn about when, how and by whom any changes to the task were made.

### Tag descriptions

There might be an optional `tasks/tags` file with the following format:

```
<tag-name> , <tag-description>
<tag-name> , <tag-description>
<tag-name> , <tag-description>
...
```

It serves as a documentation for each existing tag and the thirdparty tools may use it to display the tag descriptions.
