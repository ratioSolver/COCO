## Create a certificate for the server

To create a certificate for the server, you can use the following steps:

1. **Generate a private key**:
   ```bash
   openssl genrsa -out key.pem 2048
   ```
2. **Generate a certificate signing request (CSR)**:
   Create a configuration file `cert.cnf` with the necessary details, then run:
   ```bash
   openssl req -new -key key.pem -out cert.csr -config cert.cnf
   ```
3. **Generate a self-signed certificate**:
   ```bash
   openssl x509 -req -in cert.csr -signkey key.pem -out cert.pem -days 365 -extfile cert.cnf -extensions req_ext
   ```
