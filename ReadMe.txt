The project contains 3 files: client.c, AddressServer.c and PKServer.c
To compile them, use the command 'gcc -o name file.c'

Client
When you run the client file the first time, you will be asked to enter the following:
1. user name. This should be any integer between 1 and 10
2. private key
3. public key
A port address is given automatically from 27002 (any free port)
The client will first register with the public key server then the address server
A menu will appear:
1. Show logged in users (Enter respective number to choose)
2. connect to another user; then choose the user you want to connect to by entering thir user id
  The other client will receive a notification;to accept, choose 8; to decline choose 9
  Once connected, another menu shows:
  1.send message; then you enter your message and press enter
  2.close connection. gets you disconnected.
3. Quit. Program terminates

When a new user logs in, a notification appears
Only 5 clients should connect at a time

Address server
This registers and removes client addresses; it also sends a notification to all logged in users once another user
logs in

Public key server
This This stores and retrieves all public keys of the clients
