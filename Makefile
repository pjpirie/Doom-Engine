all: main

main: main.c
	gcc -o main main.c -lGL -lGLU -lglut -lm -ldl
