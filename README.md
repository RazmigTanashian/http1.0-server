# http1.0-server

## Purpose
This project is meant to be for educational purposes and to get more practice using C. Learning basic network stuff, string handling in C, and clean error handling.

## What does this server support?
The server currently supports GET, POST, and HEAD requests that come from a client. It can respond to the client's request with statuses 200, 204, 500, & 501.

## Building
Currently this project does not use a makefile. To build manually, first `cd` to the root of the repo's directory. Then run..
```
gcc -I include/ src/main.c src/http.c src/server.c
```

## Testing
After the executable has been built, go ahead and run it. `curl` will be used as a client-connection to the server. Open a second terminal window and run..

```
curl --http1.0 -i 127.0.0.1:8080/index.html
```

The client window should receive a response to its GET request showing the header and the contents of the index.html file.
