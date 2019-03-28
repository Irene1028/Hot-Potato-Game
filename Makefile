TARGETS=ringmaster player

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

player: player.c potato.h
	gcc -g -o $@ $<

ringmaster: ringmaster.c potato.h
	gcc -g -o $@ $<
