## Create a private key and self-signed certificate for the server

To quickly generate a private key and a self-signed certificate with subject and SANs use:

```bash
openssl req -x509 -nodes -days 365 \
    -subj "/C=IT/O=PSTLab/CN=10.0.2.2" \
    -newkey rsa:2048 \
    -keyout key.pem \
    -out cert.pem \
    -addext "subjectAltName=IP:10.0.2.2" \
    -addext "basicConstraints=CA:true"
```

This command creates both the private key (`key.pem`) and the certificate (`cert.pem`) in one step, using the subject and SANs specified directly in the command line.