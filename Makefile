debug ?= yes

ifeq ($(debug),yes)
    CPPFLAGS += -DPROJ_DEBUG
    suffix := .debug
else
    ifeq ($(debug),no)
        CXXFLAGS += -O3
        suffix := .opt
    else
        $(error debug should be either yes or no)
    endif
endif

sources := main.cpp engine.cpp cube.cpp textured_quad.cpp log.cpp shader_functions.cpp imgui_opengles_impl.cpp imgui.cpp imgui_demo.cpp imgui_draw.cpp
objects := $(addprefix ., $(sources:.cpp=$(suffix).o))
deps := $(addprefix ., $(sources:.cpp=$(suffix).d))

LIBS += 
CPPFLAGS +=
LDFLAGS += -s FULL_ES3=1 --shell-file shell_minimal.html

CXX = em++
CXXFLAGS += -pedantic -std=c++17 -g -Wall -Wno-reorder -Wno-sign-compare -Wno-address -I /usr/include/glm -s USE_WEBGL2=1 -s FETCH=1 -s WASM=0 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS='["_main", "_loadImage"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'


all : proj

proj : $(objects)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(objects) $(LIBS) -o proj.html

-include $(deps)

.%$(suffix).o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MD -MP -MF $(addprefix ., $(<:.cpp=$(suffix).d)) -c -o $@ $<

check: test
test:
	cd ../test && ./run

clean:
	rm -f .*.o .*.d

.PHONY: check clean

