.SUFFIXES: .cpp .o

CXX=g++

SRCDIR=src/
INC=include/
LIBS=lib/

#SRCS:=$(wildcard src/*.cpp)
#OBJS:=$(SRCS:.cpp=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.cpp
TARGET_OBJ:=$(SRCDIR)main.o

# Include more files if you write another source file.
SRCS_FOR_LIB:=$(SRCDIR)bpt.cpp $(SRCDIR)file.cpp $(SRCDIR)buffer.cpp $(SRCDIR)lock.cpp $(SRCDIR)trx.cpp $(SRCDIR)log.cpp $(SRCDIR)recovery.cpp
OBJS_FOR_LIB:=file.o bpt.o buffer.o lock.o trx.o log.o recovery.o

CXXFLAGS+= -g -fPIC -I $(INC)

TARGET=main

$(TARGET): $(TARGET_OBJ)
	$(CXX) $(CXXFLAGS) -c $(SRCS_FOR_LIB)
	make static_library
	$(CXX) $(CXXFLAGS) -o $@ $^ -L $(LIBS) -lbpt -lpthread

clean:
	rm $(TARGET) $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)*

library:
	g++ -shared -Wl,-soname,libbpt.so -o $(LIBS)libbpt.so $(OBJS_FOR_LIB)

static_library:
	ar cr $(LIBS)libbpt.a $(OBJS_FOR_LIB)
