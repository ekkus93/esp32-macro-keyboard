# Proposal: the device stores the repository as an opaque blob

**Status:** Direction agreed in conversation with Phil, 2026-08-03. Not
implemented. Needs his confirmation on the four open numbers below, then a
specification amendment, then code.
**Supersedes:** `PROPOSAL_2026-08-03_PACKAGE_REPOSITORY_MODEL.md` in almost
every part. See "What this obsoletes".

## The change

Phil, 2026-08-03:

> "I think that we're trying to do way too much in the esp32 and that's really
> unnecessary. Its main job is that it needs to emulate a usb keyboard and send
> macros as a usb keyboard to the computer. The second job is to save a copy of
> all of the macro data so it persists after the esp32 is turned off. Everything
> else will be handled on the client side with React."

and

> "Just store it in a blob to make things simple. We can save a couple of older
> versions. It should be compressed anyway so it shouldn't take up that much
> space."

The device has two jobs: **be a keyboard**, and **hold a blob**. It does not
parse the blob, does not know what a package is, and does not maintain an index.

## What the device keeps

| | lines today |
| --- | ---: |
| macro parser and executor | 1,541 |
| USB HID | 549 |
| auth | 792 |
| Wi-Fi, setup, provisioning, controls, app startup | ~3,700 |
| web server, minus the package and macro handlers | ~5,900 |
| storage, reduced to blob read/write | small |

## What it stops doing

Measured, not estimated — these are whole files that lose their reason to exist:

```text
storage_repository_packages.c            per-package CRUD
storage_repository_package_operations.c  duplicate, reorder, select
storage_repository_macros.c              per-macro CRUD
storage_repository_index.c               the index
storage_repository_document.c            load/store one package file
storage_repository_json.c                package file JSON
storage_repository_objects_json.c        object JSON
storage_package*.c  (8 files)            validate, export, import, replace,
                                         backup, restore, reader, writer
web_api_packages.c                       package routes
web_api_macros.c                         macro routes
                                  -----
                                  4,809 lines
```

About a quarter of the firmware, and the quarter that produced every defect
found in this session: the reorder that cleared the active package, the restore
that cleared it again, the six duplicated parsers, the two routes doing one job.

## What this obsoletes

**The backup repository, the marker, and boot recovery — all of it.** That was
designed earlier today, in detail, and it is not needed.

A repository stored as one file is replaced by one `rename()`, which §13.4
already makes atomic. A crash leaves the previous blob untouched. That *is* the
rollback Phil asked for, with no second copy to synchronise, no `applying` /
`backing-up` states, no repair-direction problem, and nothing to run at boot.

Keeping a few older versions replaces it with something better: an interrupted
write cannot lose data, **and** the user can go back deliberately.

Also obsolete: per-package checksums as a concurrency token (already removed
from the wire), the repository checksum built from package checksums (there are
no package checksums — one blob, one checksum), and the whole
package-versus-repository document distinction.

## Storage layout

```text
/data/
└── repository/
    ├── 000000007.bin      newest
    ├── 000000006.bin
    └── 000000005.bin      oldest kept
```

A monotonic counter in the filename; the highest number is current. Writing is
write `<n+1>.bin.tmp`, `rename()`, then unlink anything older than the last N.
The rename is the commit point. Nothing else needs to be atomic, because
nothing else changes.

No index file. No metadata file. The blob is the only state.

## Open, and needing Phil's answer

1. **How many versions?** Five costs 6,660 bytes at today's size — 1.3% of the
   partition. Recommendation: **5**.
2. **Who compresses?** Recommendation: **the client**. The device stores bytes
   it never interprets, so it needs no compressor at all, and "opaque" stays
   genuinely opaque. If the device compressed, it would need miniz and would
   have to understand the payload enough to know it had not already been
   compressed.
3. **How does execution get its macro?** If the device cannot read the blob it
   cannot look a macro up, so the client must send the macro **source** with the
   execute request rather than an id. Recommendation: **send the source**. The
   device compiles exactly what it was handed, which is also what it already
   does after the lookup.
4. ~~Does the `validate` endpoint stay?~~ **Decided: no.** Phil, 2026-08-03:
   "For validation feedback, the validation is moved to React." The endpoint
   goes and §25 criterion 7 is met by the client.

   **This leaves two implementations of the macro language**, and that is
   unavoidable rather than an oversight: the device must still compile source
   into keystrokes to type it, so the C parser stays whatever the client does.
   The risk is drift — the client accepts a macro, the device refuses it at
   execute time, and the user is told their macro is fine right up until it does
   nothing.

   The mitigation is not a third endpoint but a **shared conformance corpus**:
   one checked-in file of macro sources with their expected compiled output and
   expected errors, exercised by both the C host tests and the vitest suite.
   Drift then fails CI instead of surfacing on someone's keyboard. §10 is the
   contract both implementations answer to; the corpus is how that is enforced
   rather than assumed.

   A disagreement also stays visible at runtime: the device returns the parse
   error and its offset when a compile fails at execute time, so the failure
   reports itself rather than typing nothing.

## What this costs, stated plainly

- **A corrupt blob loses everything**, where today one bad file costs one
  package. Mitigated by the write being atomic, by keeping older versions, and
  by the client holding its own copy — but the blast radius genuinely changes.
- **The device can no longer enforce limits or reject malformed data.** Goal 10
  ("reject malformed or unsafe state rather than silently substituting
  defaults") becomes the client's responsibility for repository content. The
  device can still enforce a byte ceiling on the blob, and still rejects invalid
  macro *source* at execute and validate time.
- **Two clients clobber each other wholesale**, not per package. Already
  accepted when client-side concurrency control was removed, and older versions
  now soften it.
- **The web application becomes the product.** A user with no browser has a
  device that types nothing. That is already true today.

## Specification impact

Large and deliberate: §8 (the flows), §12 (the data model the device no longer
owns), §13 (layout, atomicity, corruption), §17 (routes), §25 (acceptance).
This is a redefinition of the division of labour rather than an adjustment, and
should be one reviewed amendment rather than several incidental ones.
