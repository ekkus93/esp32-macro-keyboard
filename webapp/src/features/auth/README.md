# Authentication Feature

The login form is connected to `/api/v1/auth/login`, stores the returned CSRF token
in application memory, and displays structured API failures.

First-run provisioning is currently a refusal/status screen because persistent
secure provisioning is not implemented in firmware. Logout controls, session
introspection, password changes, and automatic session-expiry navigation are still
open frontend work.

Do not add mock success paths for authentication. A failed or unavailable API must
remain visible to the user.
