#Makefile to build BACnet Application using GCC compiler

# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc
# AR = ar
# MAKE = make
# SIZE = size
#
# Assumes rm and cp are available

# Directories
BACNET_PORT ?= linux
BACNET_PORT_DIR = ../ports/${BACNET_PORT}
BACNET_INCLUDE = ../include
BACNET_OBJECT = object
BACNET_HANDLER = handler
JSON_INC_DIR=../json-c/include
JSON_LIB_DIR=../json-c
# BACnet Library
BACNET_LIB_DIR = ../lib
BACNET_LIB_NAME = bacnet
BACNET_LIB_TARGET = $(BACNET_LIB_DIR)/lib$(BACNET_LIB_NAME).a
# Compiler Setup
INCLUDE1 = -I$(BACNET_PORT_DIR) -I$(BACNET_OBJECT) -I$(BACNET_HANDLER) -I$(JSON_INC_DIR)
INCLUDE2 = -I$(BACNET_INCLUDE)
INCLUDES = $(INCLUDE1) $(INCLUDE2)
BACNET_LIB=-L$(BACNET_LIB_DIR) -l$(BACNET_LIB_NAME) -L$(JSON_LIB_DIR)
ifeq (${BACNET_PORT},linux)
PFLAGS = -pthread
TARGET_EXT =
SYSTEM_LIB= -lc -lgcc -lrt -lm -ljson-c
endif



# put all the flags together
CFLAGS := -Wall $(DEBUGGING) $(OPTIMIZATION) $(INCLUDES) $(DEFINES)
LFLAGS :=  $(BACNET_LIB) $(SYSTEM_LIB)
#CFLAGS += -I$(JSON_C_DIR)/include/json-c
#LDFLAGS += -L$(JSON_C_DIR)/lib -ljson-c
# Executable file name
TARGET = bacrp

TARGET_BIN = ${TARGET}$(TARGET_EXT)

SRCS = main.c epics.c iot_json_formatter_d2c.c iot_sockinterface.c\
	object/device-client.c

OBJS = ${SRCS:.c=.o}

all: ${BACNET_LIB_TARGET} Makefile ${TARGET_BIN}

${TARGET_BIN}: ${OBJS} Makefile ${BACNET_LIB_TARGET}
	${CC} ${PFLAGS} ${OBJS} ${LFLAGS} -o $@
	size $@
	cp $@ ../bin

lib: ${BACNET_LIB_TARGET}

${BACNET_LIB_TARGET}:
	( cd ${BACNET_LIB_DIR} ; $(MAKE) clean ; $(MAKE) )

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -rf core ${TARGET_BIN} ${OBJS} ${BACNET_LIB_TARGET}

include: .depend
