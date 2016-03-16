BUILD_DIR	= build
TARGET_DIR	= $(BUILD_DIR)/target
OBJS_DIR	= $(BUILD_DIR)/objs
OPTIONS		=  -std=c++11 -Wall -Wextra -pipe -O2 -fPIC
INCLUDES	= -Isrc
LIBS		= -lboost_program_options

all:
	mkdir -p $(TARGET_DIR)
	mkdir -p $(OBJS_DIR)
	clang++ -c src/util.cxx -o $(OBJS_DIR)/util.o $(OPTIONS) $(INCLUDES)
	clang++ -c src/util_options.cxx -o $(OBJS_DIR)/util_options.o $(OPTIONS) $(INCLUDES)
	clang++ -shared -Wl,-soname,libcxxtestutil.so -o $(TARGET_DIR)/libcxxtestutil.so \
		$(OBJS_DIR)/util.o \
		$(OBJS_DIR)/util_options.o
