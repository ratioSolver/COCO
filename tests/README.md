## Create a private key and self-signed certificate for the server

To quickly generate a private key and a self-signed certificate with a custom subject and Subject Alternative Names (SANs), use the following command:

```bash
openssl req -x509 -nodes -days 365 \
    -subj "/C=IT/O=PSTLab/CN=10.0.2.2" \
    -newkey rsa:2048 \
    -keyout key.pem \
    -out cert.pem \
    -addext "subjectAltName=IP:10.0.2.2" \
    -addext "basicConstraints=CA:true"
```

This command generates a 2048-bit RSA private key (`key.pem`) and a self-signed certificate (`cert.pem`) valid for 365 days. The certificate's subject and SANs are set directly in the command. Replace `10.0.2.2` with your server's actual IP address as needed; this value is used for both the Common Name (CN) and the SAN field.

## Port Proxy Configuration

To forward traffic from a local port to a remote server on Windows, run:

```bash
netsh interface portproxy add v4tov4 listenport=8080 listenaddress=0.0.0.0 connectport=8080 connectaddress=192.168.160.153
```

Here, `192.168.160.153` should be replaced with the IP address of the server running the CoCo service (such as a WSL server). This command forwards all connections to local port `8080` to port `8080` on the specified server.
