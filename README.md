# fivewords
Before you go any further, watch [this excellent video](https://www.youtube.com/watch?v=_-AfhLQfb6w) from Matt Parker and (of course!) subscribe to his channel.

This implementation builds on the work of [Benjamin Paassen](https://gitlab.com/bpaassen/five_clique) and [Neil Coffey](https://github.com/neilcoffey) for (at the time of writing) the fastest implementation of a 5-words-using-25-letters search.  Its improvements include:

- Written in C using an optimized bruteforce algorithm (similar to Benjamin's work) with custom-written list comparison routines
- Aggressive pruning of branches during an O(N^5) search, using Neil's trick of recalculating the dictionary for every 2-word pair (to reduce the number of comparisons)
- Bitfield-based set comparisons (also used by Neil)
- Extremely wasteful of memory so as to avoid needing to convert between sets
- Multithreading during the search phase

To use:

    wget https://github.com/dwyl/english-words/blob/master/words_alpha.txt
    gcc -O3 -Wall -Werror -o fivewords fivewords.c
    time ./fivewords words_alpha.txt <number of threads you can run>
On my machine (i9-11900H) using 16 threads it finds all 538 solutions in 1.058 seconds - over 2.5 million times faster than Matt's code :)

There's still some room for improvement here:

- There's a lot of repeated pointer dereferences in the fast loops.  Caching variables locally could trade off yet more stack space for compute time.
- I've had lots of fun with [auto-vectorization](https://gcc.gnu.org/projects/tree-ssa/vectorization.html) before, it could probably be applied here if you're careful.
- The initial search for neighbours isn't parallelized.
