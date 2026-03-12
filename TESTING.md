# Testing Methodology

A hybrid testing strategy is used that combines **system testing / end-to-end (E2E) testing**, elements of **integration testing**, plus **contract testing** and **cross-checking (differential-style testing)** across two execution paths.

Each test scenario runs in two modes (**dual-path execution**):

1. **In-process harness test**: the application entry point (`main`) is renamed/swapped and invoked **inside the test framework process**. Arguments are passed in the same shape they would have from the command line (**argv/argc semantics**). This mode provides fast feedback in a controlled environment and makes failures easier to pinpoint.

2. **Out-of-process black-box CLI test**: the same arguments are sent to the **actual compiled binary**, which is launched as a separate process (fork/spawn) by the test framework. This is a true **black-box system/E2E test**, as close as possible to how the CLI is used in real life.

Both modes use the same test oracles and checks:

* **Output contract verification**: `stdout/stderr` and exit codes are treated as part of the CLI interface contract and matched against expected patterns using **PCRE2 regular expressions** (regex-based assertions / CLI output contract testing).
* **State verification**: since the program creates or mutates a database, post-conditions are asserted via **database state checks**—validating DB contents after the scenario completes.

A scenario is considered correct only when the observed behavior agrees in both modes. This is the basis of **cross-checking**: the “in-process” and “black-box” runs must conform to the same output contract and produce the same expected data state.

The loop is further strengthened with dynamic analysis: **both the test framework and the code under test** are built and executed with sanitizers—**AddressSanitizer (ASan)** and **UndefinedBehaviorSanitizer (UBSan)**. This enables runtime detection of memory leaks, invalid memory access, and **undefined behavior** in **both** the in-process run and the out-of-process run (runtime bug detection / dynamic analysis).

As a separate step, **test coverage** (**line coverage** and **branch coverage**) is analyzed and an **HTML coverage report** is generated, serving as a quality artifact and a navigable map of what new or changed code is actually exercised.

# Does It Hold Up in the Real World?

I don’t like relying on **unit tests alone**. They’re great for quickly validating individual functions and branches, but by themselves they don’t guarantee the same logic will behave correctly in the real program—with real command-line arguments, environment, I/O, and side effects. It’s common to see a unit test hit a branch and pass, while a real run never reaches that branch due to other control flow or subtle differences in execution context.

That’s why the main emphasis is on **system/integration (E2E/CLI) tests** that launch the program the way users actually do. These tests do a better job confirming that the relevant branches execute in a “production-like” scenario, and coverage analysis (especially **branch coverage**) helps reveal code that never ran even once. That’s a signal: either an important scenario is missing from tests, or the code is dead/unreachable and should be revisited.

It’s also important to look at coverage **per test suite**. If you only look at aggregate coverage across everything, unit tests can “cover” lines and branches that your integration/E2E tests never touch—and it’s easy to miss that gap. Split coverage gives a more honest picture: what’s validated by real end-to-end runs versus what’s only validated in isolation.

None of this is an argument against unit tests. They’re valuable when you need fast, precise checks for small pieces of logic, edge cases, and rare failure modes that are hard or expensive to reproduce end-to-end (including via mocks/fakes). The point is that each testing layer should add unique value instead of duplicating the same scenario without a reason.

