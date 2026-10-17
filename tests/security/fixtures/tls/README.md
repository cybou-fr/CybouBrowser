# Local TLS fixture

The certificate and private key in this directory are **public test data**, generated only for a loopback test server. They identify no user, service or production endpoint. Never deploy this key or install the certificate into a system trust store.

Generated with OpenSSL `req -x509 -newkey rsa:2048 -nodes -days 3650`, subject `CN=CYBOU test-only localhost`, SAN `IP:127.0.0.1,DNS:localhost`. Tests run from the repository root and fail if these fixtures or a Qt TLS backend are missing. OpenSSL is only needed to regenerate the files, not to run the tests.

The ordinary isolated profile rejects this self-signed certificate. A separate positive-control test page accepts only the exact leaf SHA-256 at the randomly selected loopback port; it proves that the server and WSS message path work. Another profile must still reject WSS after that exception. This test-only handler is not part of the production browser, and no global ignore-certificate-error flags are used.

`src/security/TlsBoundaryTests.h` uses only existing Qt Network/Core/WebEngine dependencies. TLS protocol/cipher, expiry, hostname and revocation qualification remain separate work. Regenerating the fixture changes its fingerprint and requires new test evidence.
