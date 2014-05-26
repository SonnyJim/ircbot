CC	= gcc
INCLUDE	= -I/usr/include
CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe -std=c99 -O3
LDFLAGS	= -L/usr/lib 
LDLIBS	= -lircclient

all: ircbot

ircbot: ircbot.o
	@echo [link]
	@$(CC) -o $@ ircbot.o $(LDFLAGS) $(LDLIBS)

quizbot: quizbot.o
	@echo [link]
	@$(CC) -o $@ quizbot.o $(LDFLAGS) $(LDLIBS)

clean:
	rm ircbot.o quizbot.o
