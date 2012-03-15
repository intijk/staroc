all:my-pic staroc deploy

run:
	cd run
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
	mpicc -c staroc.c
	mpicc master.o slave.o staroc.o -o staroc-bin

master: master.c
	mpicc -c master.c 
slave:  slave.c
	mpicc -c slave.c
clean:
	rm -rf staroc-bin *.o

