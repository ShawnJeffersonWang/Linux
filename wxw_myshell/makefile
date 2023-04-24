src = $(wildcard *.c)
obj = $(patsubst %.c, %, $(src))

myArgs= -Wall -g -lreadline

ALL:$(obj)

%:%.c
	gcc $< -o $@ $(myArgs)

clean:
	-rm -rf $(obj)

.PHONY: clean ALL
