# compiler 
COMPILE=gcc -c
LINK=gcc -o

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

# retrieve shell path
SHELL_DIR=$(realpath ./exemplo)

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

# lib compiler flags
LC_FLAGS=-Wall -g -I$(INC_DIR)

# shell compiler flags
SC_FLAGS=-Wall -g -I$(INC_DIR) -L$(LIB_DIR) -lt2fs

all: $(BIN)
	ar -cvq $(LIB) $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	$(COMPILE) -o $@ $< $(LC_FLAGS)

.PHONY: install
.PHONY: debug
.PHONY: clean
.PHONY: shell
.PHONY: dev

install: $(LIB) $(INC_DIR)/t2fs.h
	@install -t /usr/lib $(LIB)
	@install -t /usr/include $(INC_DIR)/*

shell: $(SHELL_DIR)/shell2.c
	$(LINK) $@ $< $(SC_FLAGS)

dev: $(SHELL_DIR)/dev_test.c
	$(LINK) $@ $< $(SC_FLAGS)

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
