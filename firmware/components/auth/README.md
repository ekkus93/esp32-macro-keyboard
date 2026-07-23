# Authentication Component

This component currently implements:

- PBKDF2-HMAC-SHA-256 password records with random salts
- bounded login throttling
- RAM-only expiring sessions
- random CSRF tokens
- constant-time token comparison
- explicit error propagation

Production credential provisioning and encrypted persistent configuration are not
implemented yet. Production startup must continue to refuse the network path until
secure provisioning exists. Fixed default credentials, plaintext password storage,
and unauthenticated fallback modes are prohibited.

The web server owns HTTP parsing and cookies; this component owns authentication
state and verification.
