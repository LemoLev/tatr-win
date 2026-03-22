# `tatr ls` relative paths are broken

- STATUS: OPEN
- PRIORITY: 100
- TAGS: bug

Repro:

```console
[streamer@markov thirdparty]$ pwd
/home/streamer/Programming/tsoding/tatr/thirdparty
[streamer@markov thirdparty]$ tatr ls | head
../asks/20260312-174535/TASK.md:1: [PRIORITY: 100] Document the filter language
../asks/20260315-160917/TASK.md:1: [PRIORITY: 100, TAGS: bug] When you have several TASK.md opened, it's unclear which issues they belong to
../asks/20260315-160715/TASK.md:1: [PRIORITY: 100, TAGS: stream] Build a graph of issues crossreferring to each other
../asks/20260310-023343/TASK.md:1: [PRIORITY: 90 ] Description for tags
../asks/20260304-115038/TASK.md:1: [PRIORITY: 90 ] Priority becomes irrelevant when the task is closed
../asks/20260308-171346/TASK.md:1: [PRIORITY: 80 ] There should be a command that reports all the skipped
weird folders and files found in the tasks/ folder
../asks/20260304-182334/TASK.md:1: [PRIORITY: 70 , TAGS: bug] Accidental double `tatr new` in Emacs
../asks/20260316-142209/TASK.md:1: [PRIORITY: 50 ] Subtasks
../asks/20260305-120316/TASK.md:1: [PRIORITY: 50 ] An ability to actually store `tasks` folder outside of
the main repo
../asks/20260305-120049/TASK.md:1: [PRIORITY: 50 , TAGS: release] Presentable to GitHub README
```
