OBJS=main.o app.o third-party/third-party.o \
	 $(patsubst %.cpp,%.o, $(wildcard utils/*.cpp) $(wildcard my_vulkan/*.cpp) $(wildcard my_vulkan/core/*.cpp) \
	$(wildcard my_vulkan/shaders/*.cpp) \
	$(wildcard my_vulkan/core/passes/black_hole/*.cpp))

SHDIR:=my_vulkan/shaders

V10_COMP=$(SHDIR)/black_hole_ray_marching_rk4.comp \
		$(SHDIR)/black_hole_ray_marching_rk2.comp \
		$(SHDIR)/black_hole_ray_marching_rk1.comp

V12_COMP=$(SHDIR)/black_hole_ray_query.comp \
		 $(SHDIR)/black_dihole.comp

V10_SHADERS=$(patsubst %.comp,%.comp.spv, $(V10_COMP))
V12_SHADERS=$(patsubst %.comp,%.comp.spv, $(V12_COMP))


LIBS=-lm
LIBS+=$(shell pkg-config --libs glfw3)

CFLAGS=-I./ -I./my_vulkan/shaders/ -std=c++20
CFLAGS+=-DVULKAN_DEBUG_DISABLED
CFLAGS+=-DBLACK_HOLE_RAY_QUERY
CFLAGS+=-DBLACK_DIHOLE
CFLAGS+=-DRAY_QUERY
CFLAGS+=-DVK_NO_PROTOTYPES

VulkanNonEuclidean: $(OBJS)
	g++ -o $@ $^ $(LIBS)

%.o: %.cpp
	g++ -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJS)
	rm -f VulkanNonEuclidean
