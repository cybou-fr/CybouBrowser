# Local TLS fixture

The certificate and private key in this directory are **public test data**, generated only for a loopback test server. They identify no user, service or production endpoint. Never deploy this key or install the certificate into a system trust store.

Generated with OpenSSL `req -x509 -newkey rsa:2048 -nodes -days 3650`, subject `CN=CYBOU test-only localhost`, SAN `IP:127.0.0.1,DNS:localhost`. Tests run from the repository root and fail if these fixtures or a Qt TLS backend are missing. OpenSSL is only needed to regenerate the files, not to run the tests.

The ordinary isolated profile rejects this self-signed certificate. A separate positive-control test page accepts only the exact leaf SHA-256 at the randomly selected loopback port; it proves that the server and WSS message path work. Another profile must still reject WSS after that exception. This test-only handler is not part of the production browser, and no global ignore-certificate-error flags are used.

`src/security/TlsBoundaryTests.h` uses only existing Qt Network/Core/WebEngine dependencies. TLS protocol/cipher, expiry, hostname and revocation qualification remain separate work. Regenerating the fixture changes its fingerprint and requires new test evidence.

## Certificate validity matrix

`test-ca-cert.pem` and `test-ca-key.pem` are another public test-only CA pair. `valid-cert.pem`, `expired-cert.pem` and `mismatch-cert.pem` are signed leaves using `localhost-key.pem`. The expired leaf covers 2020-01-01 through 2021-01-01; valid/name-mismatch leaves cover 2026-01-01 through 2036-01-01. Valid SANs are IP 127.0.0.1 and DNS localhost; the mismatch leaf has only DNS wrong.invalid. The test checks time preconditions so an outdated positive fixture fails instead of producing a false success.

Regenerate with `python tools/security/generate_tls_matrix.py --openssl PATH_TO_OPENSSL`. Regeneration replaces public test fixtures and requires new evidence. OpenSSL is not called by browser or test executables.

The matrix uses Qt 6.10+ profile-local additional CA trust and off-the-record profiles. No certificate-error bypass is accepted in this matrix: valid HTTPS/WSS must work; date/name/authority failures must be rejected. A final profile without CA trust must reject the valid leaf after the other profiles have used it. On Qt 6.9, source still compiles but this required test reports BLOCKED, not a fabricated PASS. System trust is unchanged.
