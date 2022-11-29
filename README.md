# Sovellusohjelmointi assignment 2

##Installation

Run the supplied makefile with "make install" to create two binaries, server and connection.

##Usage

Begin by starting the server. This will initialize the bank account service and allow you to connect
using the "connection" binaries. Up to four clients can be serviced at once, others will be placed in
a queue while wating for their turn. The following commands are accepted:

“l 1”: give balance of account 1
“w 1 123”: withdraw 123 euros from account 1
“t 1 2 123”: transfer 123 euros from account 1 to account 2
“d 1 234”: deposit 234 euros to account 1
“q”: quit and leave the desk.

Also pressing ctrl+c to send sigint to the client to leave works as well. Account data will be stored
inbetween starts of the server. In the server you can send the command 'l' to query the balances
of each desk (for this session).
