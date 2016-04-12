MACHINE= $(shell uname -s)
CXX= g++

### MAC OS X ###
ifeq ($(MACHINE),Darwin)

GLOW_INC= -I/usr/local/include/freetype2 -I./include
TEST_INC= -I/usr/local/include/freetype2 -I./include
TEST_LIB= -L/usr/local/lib/ -L./lib -lglow -lfreetype -framework Cocoa -framework OpenGL
GLOW_LIBDIR= ./lib
GLOW_LIBS_S= $(addprefix $(GLOW_LIBDIR)/, libglow.a)
GLOW_LIBS_D= $(addprefix $(GLOW_LIBDIR)/, libglow.so)
GLOW_OBJDIR= ./objs
GLOW_OBJS= $(addprefix $(GLOW_OBJDIR)/, glView.o glow.o)

all: $(GLOW_LIBS_S) $(GLOW_LIBS_D)

# libglow
$(GLOW_LIBS_S): $(GLOW_LIBDIR) $(GLOW_OBJS)
	ar -cq $(GLOW_LIBS_S) $(GLOW_OBJS)

$(GLOW_LIBS_D): $(GLOW_LIBDIR) $(GLOW_OBJS)
	$(CXX) -o $(GLOW_LIBS_D) $(GLOW_OBJS) -shared

$(GLOW_LIBDIR):
	mkdir -p $(GLOW_LIBDIR)

$(GLOW_OBJDIR)/%.o: ./src/mac/%.mm
	$(CXX) -c -o $@ $< $(GLOW_INC) -fPIC

$(GLOW_OBJS): | $(GLOW_OBJDIR)

$(GLOW_OBJDIR):
	mkdir -p $(GLOW_OBJDIR)


### LINUX ###
else

GLOW_INC= -I/usr/include -I/usr/include/freetype2 -I./include
TEST_INC= -I/usr/include -I/usr/include/freetype2 -I./include
TEST_LIB= -L/usr/lib64 -L/usr/lib -L/usr/lib/x86_64-linux-gnu -L./lib -lglow -lGL -lGLU -lfreetype -lX11
GLOW_LIBDIR= ./lib
GLOW_LIBS_S= $(addprefix $(GLOW_LIBDIR)/, libglow.a)
GLOW_LIBS_D= $(addprefix $(GLOW_LIBDIR)/, libglow.so)
GLOW_OBJDIR= ./objs
GLOW_OBJS= $(addprefix $(GLOW_OBJDIR)/, glow.o)

all: $(GLOW_LIBS_S) $(GLOW_LIBS_D)

# libglow
$(GLOW_LIBS_S): $(GLOW_LIBDIR) $(GLOW_OBJS)
	ar -cq $(GLOW_LIBS_S) $(GLOW_OBJS)

$(GLOW_LIBS_D): $(GLOW_LIBDIR) $(GLOW_OBJS)
	$(CXX) -o $(GLOW_LIBS_D) $(GLOW_OBJS) -shared

$(GLOW_LIBDIR):
	mkdir -p $(GLOW_LIBDIR)

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
	rm -f examples/text/text examples/text/objs/main.o $(GLOW_LIBS_S) $(GLOW_OBJS)

remake: clean all
