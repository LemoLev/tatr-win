# LLM search

- STATUS: OPEN
- PRIORITY: 10
- TAGS: stream

Integrate with the llama.cpp API. Start with some small model like bonsai or something.

## NOTE(20260829-204420)

I'm thinking of just organizing some basic loop which prompts the
model "Is this task relevant to the user prompt? Here is the prompt
and here is TASK.md content. Answer only yes or no." And just go
through the tasks.
