MACHINE= $(shell uname -s)
CXX= g++

### MAC OS X ###
ifeq ($(MACHINE),Darwin)

GLOW_INC= -I/usr/local/include/freetype2 -I./include
TEST_INC= -I/usr/local/include/freetype2 -I./include
TEST_LIB= -L/usr/local/lib/ -L./lib -lfreetype -lglow -framework Cocoa -framework OpenGL
GLOW_LIBDIR= ./lib
GLOW_LIBS= $(addprefix $(GLOW_LIBDIR)/, libglow.a)
GLOW_OBJDIR= ./objs
GLOW_OBJS= $(addprefix $(GLOW_OBJDIR)/, glView.o glow.o)

all: $(GLOW_LIBS)

# libglow
$(GLOW_LIBS): $(GLOW_LIBDIR) $(GLOW_OBJS)
	ar -cq $(GLOW_LIBS) $(GLOW_OBJS)

$(GLOW_LIBDIR):
	mkdir -p $(GLOW_LIBDIR)

$(GLOW_OBJDIR)/%.o: ./src/mac/%.mm
	$(CXX) -c -o $@ $< $(GLOW_INC)

$(GLOW_OBJS): | $(GLOW_OBJDIR)

$(GLOW_OBJDIR):
	mkdir -p $(GLOW_OBJDIR)

clean:
	rm -f examples/text/text examples/text/objs/main.o libglow.a $(GLOW_OBJS)

### LINUX ###
else

	GLOW_INC= -I/usr/include -I/usr/include/freetype2
	GLOW_LIB= -L/usr/lib64 -lGL -lGLU -lfreetype
	TEST_INC= 
	TEST_LIB= 
	GLOW_OBJDIR= ./objs
	GLOW_OBJS= 

endif

test: text

text: text/main.o
	$(CXX) -o ./examples/text/text ./examples/text/objs/main.o $(TEST_INC) $(TEST_LIB)

text/main.o: | ./examples/text/objs
	$(CXX) -c -o ./examples/text/objs/main.o ./examples/text/src/main.cpp $(TEST_INC)

./examples/text/objs:
	mkdir -p ./examples/text/objs

remake: clean all
