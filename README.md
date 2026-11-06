*This project has been created as part of the 42 curriculum by zhamza.*

# Codexion

A C / POSIX-threads resource-arbitration simulation inspired by the dining
philosophers problem. It models coders sharing USB dongles in a circular hub,
where every compilation needs **both** adjacent dongles, every release incurs a
**cooldown**, and competing requests are ordered using **FIFO or EDF**.


## Description

There are `N` coders and `N` shared dongles. Each coder loops through:

1. Acquire their two neighboring dongles **atomically**.
2. Compile while both dongles are held.
3. Release both, initiating each dongle's cooldown.
4. Debug and refactor without holding dongles, then request them again.

Every coder has a deadline: `last_compile_start + time_to_burnout`. At the
start of the simulation, `last_compile_start` is the common simulation start.
A monitor thread stops the simulation at the earliest missed deadline. It also
stops once every coder has **completed** the required number of compilations.

## Instructions

Requires a C compiler with POSIX threads, `make`, and (for the optional test
suite) Python 3. No external C libraries or libft are required.

```sh
make
./codexion 5 800 200 200 100 3 50 edf
./codexion 5 800 200 200 100 3 50 fifo
make test
make clean
make fclean
make re
```

All eight positional arguments are mandatory:

| Position | Name | Meaning |
| --- | --- | --- |
| 1 | `number_of_coders` | Positive integer; also number of dongles |
| 2 | `time_to_burnout` | Milliseconds from start of last compilation to next start deadline |
| 3 | `time_to_compile` | Milliseconds holding both dongles |
| 4 | `time_to_debug` | Milliseconds debugging |
| 5 | `time_to_refactor` | Milliseconds refactoring |
| 6 | `number_of_compiles_required` | Stop after all have completed at least this many |
| 7 | `dongle_cooldown` | Minimum milliseconds from release to reacquisition |
| 8 | `scheduler` | Exactly `fifo` or `edf` |

Invalid numbers, negatives, overflow, wrong argument count, invalid scheduler,
and zero coders are rejected. Zero durations and zero required compilations are
accepted. With zero required compilations, the program exits immediately.

### Logs

The program emits only these event forms to stdout:

```text
0 1 has taken a dongle
0 1 has taken a dongle
0 1 is compiling
200 1 is debugging
400 1 is refactoring
```

A single coder has only one dongle; it takes that dongle once and cannot start
compiling. A separate monitor eventually prints `<timestamp> 1 burned out`.

## Architecture

| File | Responsibility |
| --- | --- |
| `src/main.c` | CLI and lifecycle |
| `src/parsing.c` | Strict nonnegative, overflow-safe argument parsing |
| `src/init.c`, `src/cleanup.c` | Structures, mutexes, queues, allocation and cleanup |
| `src/threads.c` | Worker/monitor startup barrier, joins and creation failures |
| `src/worker.c`, `src/worker_phases.c` | Compilation, debugging, refactoring |
| `src/acquire.c` | Request creation and waiting |
| `src/grant.c`, `src/grant_wait.c` | Pair arbitration and cooldown wake-ups |
| `src/dongle.c` | Per-dongle mutex discipline, pair locking and release |
| `src/heap.c`, `src/heap_pop.c` | Custom binary min-heap priority queues |
| `src/monitor.c` | Precise earliest-deadline burnout detection; completed coders excluded |
| `src/edf_schedule.c` | Deterministic EDF priority tie-break; does not delay initial acquisition |
| `src/timing.c` | Monotonic elapsed time; condition-variable timed waits |
| `src/logging.c` | Serialized, direct `write(1, ...)` output to stdout |

### Dongle mapping

Coder `i` (zero-based) needs dongles `i` and `(i + 1) % N`, ensuring that
neighboring coders share exactly one dongle. For `N == 2`, both coders share
both dongles. For `N == 1`, there is only one dongle and no compilation.

### FIFO / EDF priority heaps and atomic reservations

Every dongle has a binary min-heap of requests from its neighboring coders.
One coder enqueues its request on both dongles while holding the simulation
mutex. A coder can take both dongles only when neither is held or cooling and
no higher-priority **currently pair-eligible** coder is waiting on either
one. An earlier waiter whose *other* dongle is held or cooling does not
reserve this one indefinitely; skipping such temporarily ineligible waiters
avoids head-of-line blocking. Among requests whose complete pair can be
granted at the same time, FIFO/EDF priorities are respected. The physical
acquisition and updates remain atomic under the simulation mutex, with
per-dongle mutexes acquired in ascending index order.

Each dongle has exactly two adjacent possible requesters, so its heap
contains at most two entries. `heap_remove()` supports removing a granted
eligible coder even if that coder was not at the root of the heap.

- **FIFO**: increasing request sequence, then coder ID.
- **EDF**: earliest absolute `request_deadline` first. Equal deadlines use a
  deterministic priority ordering from `edf_slot()`, followed by request
  sequence and coder ID. Despite that function name, it is **only a
  tie-break**, not a time gate: a coder never waits for `edf_slot()` to arrive.

In the current version, fixed EDF admission-window waits have been removed.
`init_schedule()` still computes a deterministic tie-break order, but it
**does not reserve** resources or delay a request. A request is granted
as soon as both eligible dongles and its queue priority permit it.

Avoid describing this as a proven globally starvation-free scheduler.
Eligible-pair bypass reduces head-of-line blocking, but this differs from a
literal non-skipping FIFO queue: an older request may be bypassed while its
other dongle is unavailable. This is an interpretation that should be
discussed with evaluators. Cooldowns can also make workloads infeasible. In particular, the correction sheet's
`5 3000 200 200 200 10 800 fifo/edf` case asks for **comparative arbitration
order**, not that every coder must finish without burnout. A burnout in that
case alone does not prove a violation of the provided benchmark. Feasible EDF
liveness is still a subject requirement and should be assessed separately.

## Blocking cases handled

### Deadlock and Coffman's conditions

The classic four Coffman conditions are mutual exclusion, hold-and-wait,
no preemption, and circular wait. Mutual exclusion is necessary for these
resources, but **hold-and-wait is prevented**: both dongles are reserved
atomically under the simulation mutex, or neither is reserved. There is no
partial dongle ownership while waiting. Per-dongle heap priorities are
strictly ordered, preventing queue-order cycles; mutex acquisitions are
ordered by dongle index.

### Starvation prevention

FIFO ranks the currently grantable requests in arrival order. EDF ranks
the currently grantable requests by earliest burnout deadline with a
deterministic tie-break. Two-dongle atomic granting
prevents partial ownership while a coder is waiting; it does not by itself
guarantee starvation freedom for every feasible workload.
After a coder has completed its required compilation count, it finishes the
current debug/refactor transition and stops requesting dongles. The monitor
no longer treats that coder as needing another compilation.

### Cooldown handling

Each dongle stores `ready_at`, assigned at release as
`monotonic_now + cooldown`. Free ownership is not sufficient to grant it;
the current monotonic time must also have reached `ready_at`.
A waiting coder sleeps on a condition variable with a timed wait until the
latest of its two candidate dongle cooldowns expires. Ordinary releases,
new requests, compilation starts, and simulation stop all notify waiters.

### Precise burnout detection

An independent monitor thread scans the coders' absolute deadlines while
holding the simulation mutex. It performs a `pthread_cond_timedwait` for the
nearest deadline, waking early when any compilation changes a deadline. When
a deadline is missed it sets the shared stop flag, logs the burnout event,
and broadcasts to all sleepers. The program uses `CLOCK_MONOTONIC` for elapsed
time and converts relative waits into `CLOCK_REALTIME` deadlines because
normal `pthread_cond_t` objects use the real-time condition clock by default.
No busy-waiting is needed. Logged timestamps have millisecond resolution;
OS scheduling may introduce small delays.

### Log serialization

A dedicated logging mutex guards each complete line. Messages are assembled
in a small stack buffer and written with `write(1, ...)`, avoiding stdio
buffering delays. Event logging is called while the simulation mutex is held,
so the stop state cannot change mid-message. No messages are printed after
an observed burnout.

## Thread synchronization mechanisms

**Locking order:** `sim.mutex` is always acquired before any dongle mutex;
for two distinct dongles, the lower index is locked first. A print mutex
may be taken while holding `sim.mutex`, never the other way round. This
fixed order avoids lock-inversion deadlocks.

- `sim.mutex`: protects the shared stop/start flags, coder compilation
  counters, deadlines, request sequence and arbitration transactions.
- `dongle[i].mutex`: protects that dongle's owner, ready-at timestamp, and
  request heap. All access to these fields happens with this mutex held.
- `print_mutex`: prevents messages being interleaved during output.
- `changed` condition: prevents CPU-intensive polling; it coordinates
  request waiters, the monitor, sleeps, the start barrier, and shutdown.

For example, two adjacent coders can simultaneously attempt to request the
same dongle. Their requests are serialized by `sim.mutex`, recorded in
per-dongle heaps, and granted only by a transaction holding the dongles'
mutexes, so the physical resource cannot be duplicated or stolen. The
monitor accesses `last_start` under the same simulation mutex, so it cannot
observe a torn update while a worker starts a compilation.

## Testing

Run `make test` for quick behavior and malformed-input checks. Run
`make test-scale` for the longer reference benchmarks from the provided
correction sheet (including 400/800ms cooldown). These tests check log format,
absence of interleaving, completion counts, inferred resource ownership and
cooldowns. They do not establish a formal proof of scheduler liveness or
thread safety. In the 800ms benchmark, burnout is reported but not automatically
treated as a failure; consult the correction sheet wording and grant order.

Use additional stress tests, the campus `norminette` checker, and memory
checkers on the target machine before submission. The tests are not a
formal proof of fairness, precise 10-ms burnout logging on every machine,
or conformance to the Norm.

## Resources

- Project subject: **Codexion v1.5**, Chapters IV–VIII.
- POSIX threads: `pthread_create(3)`, `pthread_mutex_lock(3)`,
  `pthread_cond_wait(3)`, `pthread_cond_timedwait(3)` manual pages.
- Time functions: `clock_gettime(2)` and `clock_gettime(3)` manual pages.
- Operating Systems: Three Easy Pieces, chapters on locks and condition
  variables (Remzi H. Arpaci-Dusseau and Andrea C. Arpaci-Dusseau).
- Cormen et al., *Introduction to Algorithms*, binary heaps and priority
  queues.
- Coffman et al. (1971), conditions for resource deadlocks.

### AI usage disclosure

AI assistance was used to review the subject, propose the concurrency
architecture, draft the C implementation, design a custom heap and pair
arbitration mechanism, assemble the Makefile, write documentation, and
generate initial automated tests. The generated code and explanations must
be independently reviewed, tested and understood by the submitting student,
as required by the subject. AI output is not evidence of correctness.

## Submission / defense notes

Commit source code, headers, `Makefile`, and `README.md` to your assigned
Git repository. Do not submit only a binary. Verify Norm, run on the campus
machines, and be ready to explain the locking order, both-dongle grant,
EDF tie-break, heap operations, cooldown wake-up, monitor clock, and each
termination path. Recode tasks may be requested during peer evaluation.

### FIFO -> LIFO defense practice

See `DEFENSE_LIFO.md`. The official program intentionally continues accepting
only `fifo` and `edf`; the LIFO comparator is a **temporary defense recode**,
not a third accepted command-line scheduler. Revert it before committing.
# codexion
# codexion
# code-new
