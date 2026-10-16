# CYBOU 0.7.1 adaptive omnibox integration

## Baseline (2026-10-16)

Actual local and GitHub main: `095257c3461081cebf4ad3e3ae2452fe385a0662`, ahead of the supplied audit's `9a809aa`. Download launch review/protection must be retained along with HTTPS-only. Qt 6.12.0 / MSVC 2022 x64, C++17. Release build exit 0.

Baseline logs in `build/baseline-071-*.log`: smoke exit 0; first controlbar run exit 2 at the old hover bridge assertion, retry exit 0; security exit 0. This records a pre-existing intermittent hover failure, not an unqualified all-pass baseline. The replacement must test the actual pointer corridor using window coordinates. Baseline screenshot: `build/baseline-071-preview.png`.

The two original input documents are retained in `omnibox-source/`. Their repository audit and source line numbers are historical observations. The active contract is BROWSE/EDIT in one navigation row, deterministic hidden-only overflow, one existing capability controller/popover, and preservation of the security backend and browser features. No cloud service, branch, force push or remote publication is required.
