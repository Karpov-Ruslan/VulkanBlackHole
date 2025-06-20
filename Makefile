OBJS=main.o app.o third-party/third-party.o \
	 $(patsubst %.cpp,%.o, $(wildcard utils/*.cpp) $(wildcard my_vulkan/*.cpp) $(wildcard my_vulkan/core/*.cpp) \
	$(wildcard my_vulkan/shaders/*.cpp) \
	$(wildcard my_vulkan/core/passes/black_hole/*.cpp))

LIBS=-lm
LIBS+=$(shell pkg-config --libs glfw3)

CFLAGS=-I./ -I./my_vulkan/shaders/spv -std=c++20
CFLAGS+=-DDEBUG_DISABLED
CFLAGS+=-DRAY_MARCHING_RK1
CFLAGS+=-DVK_NO_PROTOTYPES

VulkanNonEuclidean: $(OBJS)
	g++ -o $@ $^ $(LIBS)

%.o: %.cpp
	g++ -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJS)
	rm -f VulkanNonEuclidean
