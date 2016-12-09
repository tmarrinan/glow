MACHINE= $(shell uname -s)
CXX= g++
PREFIX= /usr/local

### MAC OS X ###
ifeq ($(MACHINE),Darwin)

GLOW_INC= -I/usr/local/include/freetype2 -I./include
GLOW_LIB= -L/usr/local/lib -lfreetype -framework Cocoa -framework OpenGL
GLOW_LIBDIR_S= ./lib/static
GLOW_LIBDIR_D= ./lib
GLOW_LIBS_S= $(addprefix $(GLOW_LIBDIR_S)/, libglow.a)
GLOW_LIBS_D= $(addprefix $(GLOW_LIBDIR_D)/, libglow.dylib)
GLOW_OBJDIR= ./objs
GLOW_OBJS= $(addprefix $(GLOW_OBJDIR)/, glView.o glow.o)

TEST_INC= -I/usr/local/include/freetype2 -I./include
TEST_LIB= -L/usr/local/lib -L./lib -lglow -lfreetype -framework Cocoa -framework OpenGL
TEST_BIN= ./examples/bin
TEST_EXE= $(addprefix $(TEST_BIN)/, text multi interact)
TEST_OBJS= ./examples/apps/text/objs/*.o ./examples/apps/multi/objs/*.o ./examples/apps/interact/objs/*.o

all: $(GLOW_LIBS_S) $(GLOW_LIBS_D)

# libglow
$(GLOW_LIBS_S): $(GLOW_LIBDIR_S) $(GLOW_OBJS)
	ar -cq $(GLOW_LIBS_S) $(GLOW_OBJS)

$(GLOW_LIBS_D): $(GLOW_LIBDIR_D) $(GLOW_OBJS)
	$(CXX) -o $(GLOW_LIBS_D) $(GLOW_OBJS) $(GLOW_LIB) -dynamiclib

$(GLOW_LIBDIR_S):
	mkdir -p $(GLOW_LIBDIR_S)

$(GLOW_LIBDIR_D):
	mkdir -p $(GLOW_LIBDIR_D)

$(GLOW_OBJDIR)/%.o: ./src/mac/%.mm
	$(CXX) -c -o $@ $< $(GLOW_INC) -fPIC

$(GLOW_OBJS): | $(GLOW_OBJDIR)

$(GLOW_OBJDIR):
	mkdir -p $(GLOW_OBJDIR)


### LINUX ###
else

GLOW_INC= -I/usr/include -I/usr/include/freetype2 -I./include
GLOW_LIB= -L/usr/lib64 -L/usr/lib -L/usr/lib/x86_64-linux-gnu -lGL -lGLU -lfreetype -lX11 -lrt -lpthread
GLOW_LIBDIR_S= ./lib/static
GLOW_LIBDIR_D= ./lib
GLOW_LIBS_S= $(addprefix $(GLOW_LIBDIR_S)/, libglow.a)
GLOW_LIBS_D= $(addprefix $(GLOW_LIBDIR_D)/, libglow.so)
GLOW_OBJDIR= ./objs
GLOW_OBJS= $(addprefix $(GLOW_OBJDIR)/, glow.o)

TEST_INC= -I/usr/include -I/usr/include/freetype2 -I./include
TEST_LIB= -L/usr/lib64 -L/usr/lib -L/usr/lib/x86_64-linux-gnu -L./lib/static -lglow -lGL -lGLU -lfreetype -lX11 -lrt -lpthread
TEST_BIN= ./examples/bin
TEST_EXE= $(addprefix $(TEST_BIN)/, text multi interact)
TEST_OBJS= ./examples/apps/text/objs/*.o ./examples/apps/multi/objs/*.o ./examples/apps/interact/objs/*.o

all: $(GLOW_LIBS_S) $(GLOW_LIBS_D)

# libglow
$(GLOW_LIBS_S): $(GLOW_LIBDIR_S) $(GLOW_OBJS)
	ar -cq $(GLOW_LIBS_S) $(GLOW_OBJS)

$(GLOW_LIBS_D): $(GLOW_LIBDIR_D) $(GLOW_OBJS)
	$(CXX) -o $(GLOW_LIBS_D) $(GLOW_OBJS) $(GLOW_LIB) -shared

$(GLOW_LIBDIR_S):
	mkdir -p $(GLOW_LIBDIR_S)

$(GLOW_LIBDIR_D):
	mkdir -p $(GLOW_LIBDIR_D)

$(GLOW_OBJDIR)/%.o: ./src/linux/%.cpp
	$(CXX) -c -o $@ $< $(GLOW_INC) -fPIC

$(GLOW_OBJS): | $(GLOW_OBJDIR)

$(GLOW_OBJDIR):
	mkdir -p $(GLOW_OBJDIR)

endif

test: bin text multi interact

bin:	
	mkdir -p $(TEST_BIN)

text: text/main.o
	$(CXX) -o $(TEST_BIN)/text ./examples/apps/text/objs/main.o $(TEST_LIB)

text/main.o: | ./examples/apps/text/objs
	$(CXX) -c -o ./examples/apps/text/objs/main.o ./examples/apps/text/src/main.cpp $(TEST_INC)

./examples/apps/text/objs:
	mkdir -p ./examples/apps/text/objs

multi: multi/main.o
	$(CXX) -o $(TEST_BIN)/multi ./examples/apps/multi/objs/main.o $(TEST_LIB)

multi/main.o: | ./examples/apps/multi/objs
	$(CXX) -c -o ./examples/apps/multi/objs/main.o ./examples/apps/multi/src/main.cpp $(TEST_INC)

./examples/apps/multi/objs:
	mkdir -p ./examples/apps/multi/objs

interact: interact/main.o
	$(CXX) -o $(TEST_BIN)/interact ./examples/apps/interact/objs/main.o $(TEST_LIB)

interact/main.o: | ./examples/apps/interact/objs
	$(CXX) -c -o ./examples/apps/interact/objs/main.o ./examples/apps/interact/src/main.cpp $(TEST_INC)

./examples/apps/interact/objs:
	mkdir -p ./examples/apps/interact/objs


install: $(GLOW_LIBS_D)
	cp $< $(PREFIX)/$(GLOW_LIBS_D); \
	mkdir -p $(PREFIX)/include/GLOW; \
	cp -r include/* $(PREFIX)/include/GLOW

uninstall:
	rm -f $(PREFIX)/$(GLOW_LIBS_D); \
	rm -rf $(PREFIX)/include/GLOW

clean:
	rm -f $(TEST_BIN)/* $(TEST_OBJS) $(GLOW_LIBS_S) $(GLOW_LIBS_D) $(GLOW_OBJS)

remake: clean all
