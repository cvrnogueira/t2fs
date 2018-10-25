# compiler 
CC=gcc -c

# project root dir
ROOT_DIR:=${CURDIR}

# retrieve bin dir path
BIN_DIR=$(realpath ./bin)

# make sure bin dir is set to bin 
# folder within project dir and available
# to mkdir all target
ifeq (, $(BIN_DIR))
	BIN_DIR=$(addprefix $(ROOT_DIR), '/bin')
endif

# retrieve lib path
LIB_DIR=$(realpath ./lib)

# retrieve include path
INC_DIR=$(realpath ./include)

# retrieve src path
SRC_DIR=$(realpath ./src)

# list all sources
SRC=$(wildcard $(SRC_DIR)/*.c)

# list all objects
BIN=$(addprefix $(BIN_DIR)/, $(notdir $(SRC:.c=.o))) $(LIB_DIR)/apidisk.o

# list static libs
LIB=$(LIB_DIR)/libt2fs.a

# compiler flags
CFLAGS=-Wall -g -I$(INC_DIR)

all: $(BIN)
	ar -cvq $(LIB) $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: install
.PHONY: debug
.PHONY: clean

install: $(LIB) $(INC_DIR)/t2fs.h
	@install -t /usr/lib $(LIB)
	@install -t /usr/include $(INC_DIR)/*

debug:
	@echo 'SRC     ->' $(SRC)
	@echo 'BIN     ->' $(BIN)
	@echo 'CUR_DIR ->' $(ROOT_DIR)
	@echo 'LIB_DIR ->' $(LIB_DIR)
	@echo 'INC_DIR ->' $(INC_DIR)
	@echo 'BIN_DIR ->' $(BIN_DIR)
	@echo 'SRC_DIR ->' $(SRC_DIR)

clean:
	rm -rf $(LIB_DIR)/*.a $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~
