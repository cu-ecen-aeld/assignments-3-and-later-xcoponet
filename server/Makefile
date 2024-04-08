#A default target which builds the “writer” application
#
#A clean target which removes the “writer” application and all .o files
#
#Support for cross-compilation.  
# 	You should be able to generate an application for the native build platform when GNU make variable CROSS_COMPILE is not specified on the make command line.  
# 	When CROSS_COMPILE is specified with aarch64-none-linux-gnu- (note the trailing -)your makefile should compile successfully using the cross compiler installed in step 1.

TARGET = aesdsocket

# Default target which builds the "writer" application
all: $(TARGET)

# Support for cross-compilation
ifdef CROSS_COMPILE
CC = $(CROSS_COMPILE)gcc
else
CC = gcc
endif

CFLAGS = -Wall -Werror

# Compile the source files and link the object files to create the "writer" application
$(TARGET): server.o
	$(CC) server.o -o $(TARGET)

# Compile the source file to create the object file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# writer.o: server.c
# 	$(CC) -c server.c -o server.o

# Clean target which removes the "writer" application and all .o files
clean:
	rm -f $(TARGET) *.o






