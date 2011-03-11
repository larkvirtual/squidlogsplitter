#OPT = -DDEBUG

.PHONY: clean

sls: sls.c resolver.c
	gcc $(OPT) -O3 --omit-frame-pointer -pipe \
	-o $@ $^
	strip -s $@

clean:
	/bin/rm -f sls
