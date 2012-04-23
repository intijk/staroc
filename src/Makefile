all:my-pic staroc deploy

run: deploy
	cd run/
	./staroc my-pic 4
	cd ..

deploy:my-pic staroc 
	cp staroc staroc-bin run/
	cp my-pic/my-pic_init my-pic/my-pic_compute run/

my-pic: my-pic/*.c
	cd my-pic
	make
	cd ..

staroc: staroc.c master slave
	mpicc -c staroc.c -Wall
	mpicc master.o slave.o staroc.o -o staroc-bin -Wall

master: master.c
	mpicc -c master.c  -Wall
slave:  slave.c
	mpicc -c slave.c -Wall
clean:
	rm -rf staroc-bin *.o

