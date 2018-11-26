all:socket main af_server af_client

#server socket

#server:server.c
#	gcc $^ -o $@ -lpthread
#
#socket:main.c
#	gcc $^ -o $@ -lpthread

socket:t_socket.c
	gcc -Wall -c $^
	ar -rcs libt_socket.a t_socket.o

main:main.c
	gcc -Wall $^ -o $@ libt_socket.a -lpthread -I.

af_server:af_unix_server.c
	gcc -Wall -g -o $@ af_unix_server.c

af_client:af_unix_client.c
	gcc -Wall -g -o $@ af_unix_client.c

clean:
	rm -f *.a *.o main af_client af_server

