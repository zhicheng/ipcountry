CC = cc
AR = ar
CFLAGS = -Wall -pedantic -g 
LDFLAGS = 

all: libipcountry.a ipcountry

ipcountry.o: ipcountry.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

libipcountry.a: ipcountry.o
	@$(AR) rcs $@ $^
	@echo CC $<

ipcountry: main.c libipcountry.a
	@$(CC) -o $@ $^
	@echo CC ipcountry

clean:
	@rm ipcountry.o ipcountry libipcountry.a
	@echo clean ipcountry.o ipcountry libipcountry.a
