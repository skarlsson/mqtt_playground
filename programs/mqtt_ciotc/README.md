stolen from:
https://github.com/GoogleCloudPlatform/cpp-docs-samples/tree/master/iot/mqtt-ciotc



google examples requires a eliptic curve jwt token 

generate a ec.pem like this

ssh-keygen -t rsa  -P "" -b 4096 -m PEM -f jwtRS256.key
# Don't add passphrase
openssl rsa -in jwtRS256.key -pubout -outform PEM -out jwtRS256.key.pub

now we can run that agains moscuitto (replaced url with localhost)
./mqtt_ciotc ljkfdhg --keypath ../test-keys/jwtRS256.key --projectid shiny

todo - figure how how to do auth agains jwt token in server..



