# Macro Sets Feature

Set selection, management, create, import, export, and delete screens currently
exist as routed UI states with representative content.

Persisted set listing, selection, CRUD, duplication, import, export, replacement,
ordering, and transactional deletion are not connected to the frontend yet. The
firmware has a tested set repository foundation, but the required HTTP routes are
still missing.

The UI must not report a successful mutation until the server has committed it and
returned the authoritative revision.
