# **Ta**sk **Tr**acker

This is an improvised Tasks system, because I needed something more powerful than just plane TODOs in the Source Code of my projects, yet I didn't want to install a full blown Issue Tracker System.

## The Spec

### Layout

Each project at the root has `tasks/` folder which contains sub-folders for each task.

```
project/
+-...
+-tasks/
  +-tags
  +-20260824-215300
    +-TASK.md
    +-...
  +-20260830-000403-rexim
    +-TASK.md
    +-screenshot.png
    +-...
  +-...
+-...
```

### HUID

Each task sub-folder is named with a Task ID. The Task ID format is `[0-9]{8}-[0-9]{6}`. Just grab the current date and time and use it as the Task ID. Use UTC timezone so the current timezone is irrelevant. If your Task ID collides with an existing one, just wait one second and try again.

The full format is actually `[0-9]{8}-[0-9]{6}(-[a-zA-Z0-9\\-]*)?`. So if you work in a team you can agree on unique suffixes per individual to slap at the end like `20260829-235855-rexim` or `20260829-235902-01`. Those are valid Task IDs too.

I call this ID system HUID (Human-Unique IDentifier). It is fairly unique if you generate IDs at "Human-speed". That is for me personally the speed at which I need to generate them is never below one second.

Having a Unique Task ID is beneficial under such control systems as git, because you can generate tasks in parallel branches and then relatively easily merge them together.

### TASK.md

Inside of the task sub-folder there is one mandatory file `TASK.md` which is a markdown file describing the task. The folder may contain other files as attachments to the task. `TASK.md` should link to the attachments as necessary. Try to keep the size of the attachments small, since they are going to be committed to the git repo. Use [ffmpeg](https://ffmpeg.org/) to reencode any screencast to reduce their size as necessary.

The format of `TASK.md`:

```markdown
# <title>

- STATUS: (OPEN|CLOSED)
- PRIORITY: <number>
- TAGS: <comma-separated-list-of-tags>

[description]
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

## The Tool

The repo comes with a command line tool that helps to navigate and manipulate the `tasks/` folder:

```console
$ cc -o nob nob.c
$ ./nob
$ sudo cp ./build/tatr /usr/local/bin/
$ tatr help
```

You are welcome to make your own tools.

### Query Language

Query language is used in `tatr ls` command to select a set of tasks.

#### Examples

Query everything with tag `bug`:

```console
$ tatr ls :bug
```

Everything with tag `bug`, but without tag `ui`:

```console
$ tatr ls :bug and not :ui
```

Everything that is not tagged:

```console
$ tatr ls not tagged
```

All the bugs with priority less than 50:

```console
$ tatr ls :bug and priority below 50
```

#### Syntax

Here is the [Backus–Naur form](https://en.wikipedia.org/wiki/Backus%E2%80%93Naur_form) of the language:

```
<expr>            ::= <or>
<or>              ::= <and> *('or' <and>)
<and>             ::= <compare> *('and' <compare>)
<compare>         ::= <primary> *(<compare-op> <primary>)
<compare-op>      ::= 'below' | 'above' | equal | 'not' 'below' | 'not' 'above' | 'not' 'equal'
<primary>         ::= <tag>
                    | '(' <expr> ')'
                    | '[' <expr> ']'
                    | 'not' <primary>
                    | 'any'
                    | 'tagged'
                    | 'priority'
                    | <number>
<tag>             ::= ':' 1*<any character except ','>
<number>          ::= ['-'] 1*<digit>
```

#### Reference

| Expression | Description |
|-|-|
| `<expr1:bool> or <expr2:bool>` | True when `<expr1:bool>` or `<expr2:bool>` or both are true. Just a regular [logical disjunction](https://en.wikipedia.org/wiki/Logical_disjunction). |
| `<expr1:bool> and <expr2:bool>` | True when both `<expr1:bool>` and `<expr2:bool>` are true. Just a regular [logical conjunction](https://en.wikipedia.org/wiki/Logical_conjunction) |
| `:<tag>` | True when a task's `TAGS` property contains `<tag>` |
| `not <expr:bool>` | True when `<expr:bool>` is false. |
| `tagged` | True when a task has at least one tag in its `TAGS` property. |
| `any` | Always true for any task. |
| `priority` | Priority of the task as an integer. |
| `<expr1:int> below <expr2:int>` | True when `<expr1:int>` is less than `<expr2:int>`.|
| `<expr1:int> above <expr2:int>` | True when `<expr1:int>` is greater than `<expr2:int>`.|
| `<expr1:int> not above <expr2:int>` | True when `<expr1:int>` is less or equal to `<expr2:int>`.|
| `<expr1:int> not below <expr2:int>` | True when `<expr1:int>` is greater or equal to `<expr2:int>`.|
| `<expr1:int> equal <expr2:int>` | True when `<expr1:int>` is equal to `<expr2:int>`.|
| `<expr1:int> not equal <expr2:int>` | True when `<expr1:int>` is not equal to `<expr2:int>`.|

## The License

All the code in this repo is released under [GNU General Public License, version 2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html) license unless stated otherwise (specifically, the files in [./thirdparty/](./thirdparty/) folder have their own corresponding licenses)
