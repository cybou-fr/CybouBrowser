# Pinned OWASP inputs

Retrieved 2026-10-18 from the official OWASP GitHub repositories. Exact bytes and counts are pinned in `../standards.lock.json`; the gate verifies them offline. These files are standards content, not instructions to execute.

- **ASVS 5.0.0**: `asvs-5.0.0.csv`, the English CSV release asset from [v5.0.0_release](https://github.com/OWASP/ASVS/releases/tag/v5.0.0_release); source tag commit `5cf9b032440be53ce345ab3c130fda46ba1ce7a2`. 345 requirements, identified by the `req_id` column. Original bytes retained. License: `ASVS-LICENSE.md`.
- **WSTG 4.2**: `wstg-4.2-checklist.md`, [Testing_Checklist.md](https://github.com/OWASP/wstg/blob/dd33419e10edb22b78d89325a6c2aad9f184e3a2/checklist/Testing_Checklist.md) at tag `v4.2`, commit `dd33419e10edb22b78d89325a6c2aad9f184e3a2`. 97 unique test IDs. Original bytes retained. License: `WSTG-LICENSE`.
- `wstg-4.2-ids.txt` is a derived, sorted UTF-8/LF index, one ID per line. Reproduce by matching `^\|\s*(WSTG-[A-Z]{4}-\d{2})\s*\|` with multiline mode against the pinned Markdown, sorting captured IDs and appending a final newline. The gate independently compares that source set with the index and checks both hashes.
- `wstg-4.2-checklist.json` is retained as upstream comparison evidence, **not the index source**. That same tag contains 98 JSON rows and only 97 unique IDs: both Buffer Overflow and Format String Injection use `WSTG-INPV-13`. The Markdown checklist has 97 unique rows and assigns `INPV-13` to Format String Injection. We preserve both upstream files without silently correcting their contents; no extra scenario is invented or certified through this import.

Attribution: OWASP Foundation and the ASVS/WSTG project contributors. See the supplied upstream licenses for redistribution terms. Local index derivation and coverage metadata do not change the upstream standard.

`../coverage.json` contains every imported identifier with its title. Initial disposition is `untriaged`, with no owner or case mapping asserted. Importing an ID is not evidence of applicability, testing, compliance or independent review. The release gate remains blocked until reviewed decisions and matching executed evidence exist.
