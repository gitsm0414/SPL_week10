target=w10
source=w10.c

$(target): $(source)
	gcc -o $@ $^
clean:
	rm $(target)
