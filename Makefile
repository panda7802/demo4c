all:socket main 

#server socket

#server:server.c
#	gcc $^ -o $@ -lpthread
#
#socket:main.c
#	gcc $^ -o $@ -lpthread

socket:t_socket.c
	gcc -c $^
	ar -rcs libt_socket.a t_socket.o

main:main.c
	gcc $^ -o $@ libt_socket.a -lpthread -I.

clean:
	rm -f *.a *.o main
