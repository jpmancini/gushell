all: gush

gush: gush.c
	gcc gush.c -o gush

clean:
	rm gush
