TARGET=main
OBJECTS=main.o id.o factorial.o
$(TARGET) : $(OBJECTS)
	gcc -o $(TARGET) $(OBJECTS) 
id.o : id.c
	gcc -c id.c
factorial.o : factorial.c
	gcc -c factorial.c 
main.o : main.c
	gcc -c main.c


