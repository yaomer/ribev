for files in `find . -name "*.c"`; do
    cc -c $files
done

ar -cr libserv.a server.o evll.o evthr.o evloop.o channel.o \
    buffer.o coder.o timer.o task.o hash.o queue.o vector.o \
    net.o event.o alloc.o log.o log_fmt.o \
    epoll.o kqueue.o evop.o

ar -cr libcli.a client.o evloop.o evop.o kqueue.o channel.o buffer.o coder.o \
    timer.o task.o hash.o queue.o vector.o \
    epoll.o net.o event.o alloc.o log.o log_fmt.o

chmod 0777 libserv.a libcli.a

rm *.o
