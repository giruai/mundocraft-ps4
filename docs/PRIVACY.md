# Data and publication review

The release is prepared from an explicit selection of source, patches, artwork
and license notices. The repository and attachments exclude console settings,
pairing profiles, test accounts, passwords, worlds, saves, logs, console
captures and private development history.

The review covers selected files, fresh Git history, archives, native binaries
and extracted PKG content. It searches for known environment data and patterns
for credentials and personal paths. Upstream sample keys or certificates are
classified as public fixtures only when their exact bytes match a pinned
source; entire test directories are never exempted from review.

Build paths use neutral prefixes. Release manifests use relative paths and
hashes. SDK archives, binary tools and developer caches are not attached.
These are bounded checks, not a guarantee that an automated scan can detect
every possible sensitive value.

## While playing

Public listing and approximate geolocation contact Luanti services over HTTPS.
When joining a game, its operator receives the data needed to maintain your
connection and player identity. Accounts and worlds are managed by each server.
The connection does not use PSN.

Local settings and client logs can contain player names and server addresses.
Review them before sharing. Do not attach passwords or pairing profiles to
bug reports.

The publishing GitHub account, upstream authors and their public attribution
data remain visible where needed for provenance. Those public credits are
distinct from private development-environment data.
