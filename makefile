MACHINE= $(shell uname -s)
CXX= g++

### MAC OS X ###
ifeq ($(MACHINE),Darwin)

GLOW_INC= -I/usr/local/include/freetype2 -I./include
GLOW_LIB= -L/usr/local/lib -lfreetype -framework Cocoa -framework OpenGL
TEST_INC= -I/usr/local/include/freetype2 -I./include
TEST_LIB= -L/usr/local/lib -L./lib/static -lglow -lfreetype -framework Cocoa -framework OpenGL
GLOW_LIBDIR_S= ./lib/static
GLOW_LIBDIR_D= ./lib
GLOW_LIBS_S= $(addprefix $(GLOW_LIBDIR_S)/, libglow.a)
GLOW_LIBS_D= $(addprefix $(GLOW_LIBDIR_D)/, libglow.so)
GLOW_OBJDIR= ./objs
GLOW_OBJS= $(addprefix $(GLOW_OBJDIR)/, glView.o glow.o)

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

$(GLOW_OBJDIR)/%.o: ./src/mac/%.mm
	$(CXX) -c -o $@ $< $(GLOW_INC) -fPIC

$(GLOW_OBJS): | $(GLOW_OBJDIR)

$(GLOW_OBJDIR):
	mkdir -p $(GLOW_OBJDIR)


### LINUX ###
else

GLOW_INC= -I/usr/include -I/usr/include/freetype2 -I./include
GLOW_LIB= -L/usr/lib64 -L/usr/lib -L/usr/lib/x86_64-linux-gnu -lGL -lGLU -lfreetype -lX11 -lrt -lpthread
TEST_INC= -I/usr/include -I/usr/include/freetype2 -I./include
TEST_LIB= -L/usr/lib64 -L/usr/lib -L/usr/lib/x86_64-linux-gnu -L./lib/static -lglow -lGL -lGLU -lfreetype -lX11 -lrt -lpthread
GLOW_LIBDIR_S= ./lib/static
GLOW_LIBDIR_D= ./lib
GLOW_LIBS_S= $(addprefix $(GLOW_LIBDIR_S)/, libglow.a)
GLOW_LIBS_D= $(addprefix $(GLOW_LIBDIR_D)/, libglow.so)
GLOW_OBJDIR= ./objs
GLOW_OBJS= $(addprefix $(GLOW_OBJDIR)/, glow.o)

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

test: text

text: text/main.o
	$(CXX) -o ./examples/text/text ./examples/text/objs/main.o $(TEST_LIB)

text/main.o: | ./examples/text/objs
	$(CXX) -c -o ./examples/text/objs/main.o ./examples/text/src/main.cpp $(TEST_INC)

./examples/text/objs:
	mkdir -p ./examples/text/objs


clean:
	rm -f examples/text/text examples/text/objs/main.o $(GLOW_LIBS_S) $(GLOW_LIBS_D) $(GLOW_OBJS)

remake: clean all
