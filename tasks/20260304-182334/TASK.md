# Accidental double `tatr new` in Emacs

- STATUS: OPEN
- PRIORITY: 70
- TAGS: bug

There is a weird UX problem I keep experiencing in Emacs' compilation
mode with `tatr new`. I have a habit of switching to `*compilation*`
buffer and hitting `g` to rerun the last commands. Sometimes such
commands is `tatr new` which leads to creating the same task
twice. I'm not sure what to do about it as I feel like I'm probably
the only one who will experience this problem even after the tool goes
public.

If can probably have a similar problem in Bash if you have a habit of
hitting up and enter.
