# Haar_Library

Library for the ruggedized Haar temperature, pressure, and relative-humidity sensor.

## Standards

Follow the NW library standards for all work here. Two sources:
- **Within the NorthernWidget workspace:** the root `CLAUDE.md` (one level above `github/`) has the full checklist and code conventions.
- **Fresh clone / other location:** fetch [NorthernWidget/.github/RELEASING.md](https://github.com/NorthernWidget/.github/blob/main/RELEASING.md) for the release checklist and [NorthernWidget/.github/CONTRIBUTING.md](https://github.com/NorthernWidget/.github/blob/main/CONTRIBUTING.md) for naming and versioning conventions.

This is a **sensor library**. Schema 1 migration is required once the spec is stable and a Schema 0 snapshot tag exists. Refer to the Haar device appendix in [NW-Device-Specification](https://github.com/NorthernWidget/NW-Device-Specification) for the exact Page 1 register layout.

## Hard rule

**Never** create a git tag, GitHub release, push to a shared remote, or submit to any external registry (Zenodo, Arduino Library Manager, etc.) unless explicitly asked in the current message. If in doubt, ask.
