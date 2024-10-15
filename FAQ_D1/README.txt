INSTALL BELOW THING FIRST - 
sudo apt-get install libuuid libuuid-devel
sudo apt-get install uuid-dev

Compile the code -
CLIENT code-  
gcc client.c -o client -pthread -luuid
SERVER code-
gcc server.c -o server -pthread -luuid


RUN code - 
1. First start the SERVER
./server

2. Then CLIENT
run below thing in different shell
./client

3.For Multiple client 
run below thing in different shell
./client

4.Give command - (YOU can get dest_id and recipient_id by using active command)
/active
/chatbot login
/chatbot logout
/send <dest_id> <msg>
/history <recipient_id>
/history_delete <recipient_id>
/delete_all
/logout --- FOR TERMINATION