# the compiler
CC = gcc
RM = rm
CP = cp
CHMOD = chmod

# Where to install the program?
BINDIR = /usr/bin

# Init script
INITSCRIPT = template/mbserver-initscript

# Where to put the init script?
INITSCRIPTDIR = /etc/init.d

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall

# list all include directories
INCDIRS += /usr/local/include/modbus	# for RPi
#INCDIRS += /usr/include/modbus		# for Ubuntu

# list all library directories
LIBDIRS += /usr/local/lib

# list all libraries 
LIBS += pthread modbus

# the build target executable:
TARGET = mbserver

CPPFLAGS += $(INCDIRS:%=-I %)
LDFLAGS += $(LIBDIRS:%=-L %)
LDFLAGS += $(LIBS:%=-l %)

.PHONY: clean install

all: $(TARGET)

$(TARGET): src/$(TARGET).c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $(TARGET) src/$(TARGET).c $(LDFLAGS)

clean:
	$(RM) $(TARGET)

install:
	$(CP) $(TARGET) $(BINDIR)
	$(CP) $(INITSCRIPT) $(INITSCRIPTDIR)/$(TARGET)
	$(CHMOD) +x $(INITSCRIPTDIR)/$(TARGET)
