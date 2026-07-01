# Security Policy

## Supported Versions

Security fixes are provided for the latest released version and the current `main` branch.

## Reporting a Vulnerability

Please do not open public issues for suspected vulnerabilities.

Report vulnerabilities privately via GitHub Security Advisories:
https://github.com/precizer/precizer/security/advisories/new

Please include:

* affected version
* reproduction steps
* impact
* any suggested fix

We will review the report and respond as soon as possible.

## Code Signing Policy

Free code signing provided by [SignPath.io](https://signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).

Only official MSYS2 x64 release artifacts built from this repository by GitHub Actions may be submitted for signing. Manually assembled binaries must not be submitted for production signing.

The signing flow is:

1. GitHub Actions builds the MSYS2 x64 release artifact.
2. The unsigned artifact is uploaded as a GitHub Actions artifact.
3. The artifact is submitted to SignPath for signing.
4. A project approver approves the signing request.
5. The signed artifact is verified and published on GitHub Releases.

Current release approver: [`@precizer`](https://github.com/precizer).

Maintainers with release, repository administration, or SignPath access must use multi-factor authentication.

This program will not transfer any information to other networked systems unless specifically requested by the user or the person installing or operating it.

`precizer` is a local command-line tool. It enumerates files, computes checksums, and writes results to a local database. It does not store file contents, install services, add startup entries, or modify the files it scans.
