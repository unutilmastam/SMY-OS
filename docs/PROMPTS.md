# SMY-OS — Phase prompts

The owner does not copy these prompts. Claude Code applies them automatically
(see `CLAUDE.md` → Autopilot). They are kept here so every session behaves the
same way.

---

## Standard phase prompt

> Execute Phase N of `ROADMAP.md` for SMY-OS.
>
> 1. Read `CLAUDE.md`, `docs/MASTER_PROMPT.md`, `ROADMAP.md`, `STATUS.md` and the
>    docs/ADRs for the area you touch. For design phases also read
>    `docs/DESIGN_PROMPT.md`.
> 2. Inspect the environment: run `tools/check-env.sh` (if it exists) or check
>    the needed tools manually. Install missing tools with `apt-get` if
>    possible. Note what can only run in CI.
> 3. Set the phase status to `IN PROGRESS` in `ROADMAP.md`.
> 4. Restate the phase goal and acceptance criteria. List files to change, a
>    short plan, assumptions and risks (put this in the final report, do not
>    wait for approval unless the phase says OWNER DECISION).
> 5. Implement in small steps. After each meaningful step: build and run the
>    relevant tests. Treat warnings as errors.
> 6. Add tests for everything new: host unit tests for portable logic, QEMU
>    tests for kernel behaviour. A hang must be a failure (timeouts).
> 7. Run the full check sequence (build, unit tests, format check, static
>    analysis, QEMU tests). Report the real result of each command. If a check
>    cannot run in the session, say so and rely on CI.
> 8. Update docs: `STATUS.md` (what is Implemented / Compiles / Unit-tested /
>    Emulator-tested), the component docs, `TESTING.md` matrix, `LICENSES.md`
>    for any new dependency, an ADR for each significant decision.
> 9. Commit with a clear message and push to `main`. Check that the CI run for
>    the pushed commit is green (GitHub tools). If red: fix and push again.
> 10. Set the phase to `VERIFIED` only if every acceptance criterion passed in
>     CI. Otherwise leave `IN PROGRESS` / `BLOCKED` and explain why.
> 11. Report to the owner in Uzbek (Latin), short: what was done, check
>     results, limitations, any decision needed. Last line:
>     `Keyingi: Phase N+1 — "davom et" deb yozing.`
> 12. Stop. Do not start the next phase.

---

## Error fix prompt ("xato" / "tuzat")

> The owner reports an error (text, screenshot description or CI log).
>
> 1. Reproduce it: fetch the failing CI job log with the GitHub tools or run the
>    same command locally.
> 2. Find the root cause. "Flaky" is not a root cause.
> 3. Fix minimally, add a regression test if feasible, run the full check
>    sequence, commit, push, confirm CI is green.
> 4. Never disable or delete a test to get green.
> 5. Report in Uzbek: cause, fix, test result. Repeat the `Keyingi:` line for
>    the current phase.

---

## Owner decision prompt

Used when a phase is marked **OWNER DECISION**.

> Prepare the decision for a non-technical owner on a phone: at most 3 options,
> a short Uzbek table (option · advantage · disadvantage · cost), and your
> recommendation in one sentence. Do the rest of the phase that does not depend
> on the decision. Mark the phase `BLOCKED` with reason "owner decision", ask
> the question, stop. When the owner answers, record it in an ADR and finish the
> phase.

---

## Status question ("holat" / "status")

> Read `ROADMAP.md`, `STATUS.md` and the latest CI result. Answer in Uzbek in
> at most 8 lines: current phase, what works (with status vocabulary), what is
> next. Do not change files.
