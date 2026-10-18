#!/usr/bin/env python3
"""Regenerate public loopback TLS fixtures. Never touches system certificate trust."""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--openssl',required=True,help='Path to an existing OpenSSL executable')
    parser.add_argument('--pna-leaf-only',action='store_true',help='Only issue pna-cert.pem from the existing test CA; leave other fixtures untouched')
    args=parser.parse_args()
    target=ROOT/'tests/security/fixtures/tls'
    target.mkdir(parents=True,exist_ok=True)
    def run(*arguments):
        result=subprocess.run([args.openssl,*map(str,arguments)],capture_output=True)
        if result.returncode: raise RuntimeError(result.stderr.decode(errors='replace'))
    ca_key=target/'test-ca-key.pem'; ca_cert=target/'test-ca-cert.pem'
    if not args.pna_leaf_only: run('req','-x509','-newkey','rsa:2048','-nodes','-days','3650','-subj','/CN=CYBOU public test CA',
        '-addext','basicConstraints=critical,CA:TRUE','-addext','keyUsage=critical,keyCertSign,cRLSign',
        '-keyout',ca_key,'-out',ca_cert)
    with tempfile.TemporaryDirectory() as tmp:
        base=Path(tmp); (base/'index').write_text(''); (base/'serial').write_text('01\n')
        config=base/'ca.cnf'
        config.write_text(f"""[ca]
default_ca=fixture
[fixture]
dir={base.as_posix()}
database=$dir/index
serial=$dir/serial
new_certs_dir=$dir
certificate={ca_cert.as_posix()}
private_key={ca_key.as_posix()}
default_md=sha256
policy=names
unique_subject=no
[names]
commonName=supplied
[valid]
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=IP:127.0.0.1,DNS:localhost
[pna]
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=IP:127.0.0.1,DNS:localhost,DNS:cybou-source.test,DNS:cybou-alias.test
[mismatch]
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=DNS:wrong.invalid
""",encoding='utf-8')
        csr=base/'leaf.csr'
        run('req','-new','-key',target/'localhost-key.pem','-subj','/CN=CYBOU test matrix leaf','-out',csr)
        pna=[('pna','20260101000000Z','20360101000000Z','pna')]
        for name,start,end,extension in pna if args.pna_leaf_only else pna+[('valid','20260101000000Z','20360101000000Z','valid'),
                                         ('expired','20200101000000Z','20210101000000Z','valid'),
                                         ('mismatch','20260101000000Z','20360101000000Z','mismatch')]:
            run('ca','-batch','-notext','-config',config,'-extensions',extension,
                '-startdate',start,'-enddate',end,'-in',csr,'-out',target/(name+'-cert.pem'))
    print('Generated public test-only CA and valid/expired/mismatch leaves; no trust-store changes')
if __name__=='__main__': main()
