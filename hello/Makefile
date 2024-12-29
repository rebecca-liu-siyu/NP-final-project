include ../Make.defines

PROGS =	tcpcli01 tcpcli04 tcpcli05 tcpcli06 \
		tcpcli07 tcpcli08 tcpcli09 tcpcli10 \
		tcpserv01 tcpserv02 tcpserv03 tcpserv04 \
		tcpserv08 tcpserv09 tcpservselect01 tcpservpoll01 tsigpipe \
		my_tcpcli_hw1 my_tcpcli_hw3

all:	${PROGS}

FORCE:

serv:	serv.o FORCE
		${CC} ${CFLAGS} -o $@ serv.o ${LIBS}
		./serv 8082
cli:    cli.o FORCE
		${CC} ${CFLAGS} -o $@ cli.o ${LIBS}
		./cli 127.0.0.1 8082

my_tcpcli_hw1:	my_tcpcli_hw1.o FORCE
		${CC} ${CFLAGS} -o $@ my_tcpcli_hw1.o ${LIBS}
		./my_tcpcli_hw1 140.113.235.236

my_tcpcli_hw2:	my_tcpcli_hw2.o FORCE
		${CC} ${CFLAGS} -o $@ my_tcpcli_hw2.o ${LIBS}
		./my_tcpcli_hw2 140.113.235.237

my_tcpcli_hw3:	my_tcpcli_hw3.o FORCE
		${CC} ${CFLAGS} -o $@ my_tcpcli_hw3.o ${LIBS}
		./my_tcpcli_hw3 140.113.235.236

tcpcli01:	tcpcli01.o
		${CC} ${CFLAGS} -o $@ tcpcli01.o ${LIBS}

tcpcli04:	tcpcli04.o
		${CC} ${CFLAGS} -o $@ tcpcli04.o ${LIBS}

tcpcli05:	tcpcli05.o
		${CC} ${CFLAGS} -o $@ tcpcli05.o ${LIBS}

tcpcli06:	tcpcli06.o
		${CC} ${CFLAGS} -o $@ tcpcli06.o ${LIBS}

tcpcli07:	tcpcli07.o
		${CC} ${CFLAGS} -o $@ tcpcli07.o ${LIBS}

tcpcli08:	tcpcli08.o str_cli08.o
		${CC} ${CFLAGS} -o $@ tcpcli08.o str_cli08.o ${LIBS}

tcpcli09:	tcpcli09.o str_cli09.o
		${CC} ${CFLAGS} -o $@ tcpcli09.o str_cli09.o ${LIBS}

tcpcli10:	tcpcli10.o
		${CC} ${CFLAGS} -o $@ tcpcli10.o ${LIBS}

tcpcli11:	tcpcli11.o str_cli11.o
		${CC} ${CFLAGS} -o $@ tcpcli11.o str_cli11.o ${LIBS}

tcpserv01:	tcpserv01.o
		${CC} ${CFLAGS} -o $@ tcpserv01.o ${LIBS}

tcpserv02:	tcpserv02.o sigchldwait.o
		${CC} ${CFLAGS} -o $@ tcpserv02.o sigchldwait.o ${LIBS}

tcpserv03:	tcpserv03.o sigchldwait.o
		${CC} ${CFLAGS} -o $@ tcpserv03.o sigchldwait.o ${LIBS}

tcpserv04:	tcpserv04.o sigchldwaitpid.o
		${CC} ${CFLAGS} -o $@ tcpserv04.o sigchldwaitpid.o ${LIBS}

tcpserv08:	tcpserv08.o str_echo08.o sigchldwaitpid.o
		${CC} ${CFLAGS} -o $@ tcpserv08.o str_echo08.o sigchldwaitpid.o \
			${LIBS}

tcpserv09:	tcpserv09.o str_echo09.o sigchldwaitpid.o
		${CC} ${CFLAGS} -o $@ tcpserv09.o str_echo09.o sigchldwaitpid.o \
			${LIBS}

tcpservselect01:	tcpservselect01.o
		${CC} ${CFLAGS} -o $@ tcpservselect01.o ${LIBS}

tcpservpoll01:	tcpservpoll01.o
		${CC} ${CFLAGS} -o $@ tcpservpoll01.o ${LIBS}

tsigpipe:	tsigpipe.o
		${CC} ${CFLAGS} -o $@ tsigpipe.o ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}
