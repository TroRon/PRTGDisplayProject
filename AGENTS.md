# Public project rules

This repository is a public distribution. Never copy private infrastructure inventories, hostnames, addresses, secrets, provisioned flash backups, private signing keys or private development history here. Use example.org and clearly synthetic names only. Preserve upstream licences and existing releases. The RSA public key is intentionally public; private signing keys must stay outside this repository. German documentation uses real umlauts and Swiss spelling. Changes require targeted tests and a privacy scan. Builds and local tests are not a hardware OTA/rollback acceptance test. No commit/push or production deployment without an explicit request.

## Mandatory publication boundary

Read WORKFLOW.md before making changes here. Develop candidates outside this public checkout. Before EVERY new transfer into this repository, ask the user for explicit approval of the concrete reviewed scope (files/version, tests, commit/push, release/OTA). Previous release approvals and hardware confirmations do not authorize future changes. An explicit current instruction to publish the concrete change is sufficient; never ask twice for the same approved action. Expand scope only after renewed approval.

Configure `git config core.hooksPath .githooks`. After the user approves, bind the local `.git/publication-approved-commit` marker to the exact reviewed HEAD. Never create or refresh it based on inferred approval. Never bypass the hook. GitHub API publishing also requires the same explicit approval. Sources, existing release bytes and OTA URLs must remain stable unless the approved scope explicitly changes them.

Current hardware acceptance: the operator confirmed 0.8.1 OTA installation and inactive-category filtering on 24 September 2026. This does not establish a tested rollback or authorize a future release.

Published 0.9.0: Wi-Fi scan, manual SetupPRTGDisplay hotspot and optional HTTP WebAdmin. Local builds/tests passed; physical 0.9.0 acceptance remains pending. The operator explicitly approved this publication; future changes still require fresh approval.

Published 0.9.1 with explicit approval: automatic per-device 12-digit initial WebAdmin password when credentials are absent; existing passwords preserved. Local tests passed; physical acceptance pending. Future publications still require approval.

Explicit operator cleanup: only 0.9.1 remains in public release files, GitHub Releases/tags and the OTA catalog. Private archives and Git history remain untouched. Do not restore older offers automatically.

Release 1.0.0 explicitly authorized: verified NVS factory reset with local confirmation, Info tab, 50-second demo delay. Keep free flash sector 0xC12000–0xC12FFF reserved for reset intent; partition table unchanged. Local tests passed, physical acceptance pending. Keep 0.9.1 as the prior version; do not restore removed earlier releases.

Explicit replacement approval for 1.0.0 R2: automatic setup hotspot and WebAdmin passwords of at least five bytes. Firmware version/sequence unchanged. Public release files replaced once; original retained privately. Source tag v1.0.0-r2 distinguishes this revision without rewriting v1.0.0. QR/captive portal not included. New future changes still require approval.
