# output binary
BIN := build/nat
OUTWEB := build/webgl.html

# source files
SHELL_FILE := src/platform/web/shell_minimal.html

SRCS := $(wildcard src/*.cpp)
SRCS += src/imgui/imgui.cpp src/imgui/imgui_demo.cpp src/imgui/imgui_draw.cpp src/imgui/imgui_widgets.cpp

# intermediate directory for generated object files
OBJDIR := build/obj
# intermediate directory for generated dependency files
DEPDIR := build/deps

# object files, auto generated from source files
OBJS := $(patsubst %,$(OBJDIR)/%.o,$(basename $(SRCS)))
# dependency files, auto generated from source files
DEPS := $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS)))

# compilers (at least gcc and clang) don't create the subdirectories automatically
$(shell mkdir -p $(dir $(OBJS)) >/dev/null)
$(shell mkdir -p $(dir $(DEPS)) >/dev/null)

# C++ compiler
CXX := em++
# linker
LD := em++

# C++ flags
CXXFLAGS := -std=c++17 -g -Wall -Wextra -pedantic -Isrc/glm
EMXXFLAGS := -s USE_WEBGL2=1 -s FETCH=0 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS='["_main", "_loadImageFile"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'
# linker flags
LDFLAGS :=
EMLDFLAGS := -s FULL_ES3=1 -s USE_WEBGL2=1 --shell-file $(SHELL_FILE)
# flags required for dependency generation; passed to compilers
DEPFLAGS = -MT $@ -MD -MP -MF $(DEPDIR)/$*.d

# compile C++ source files
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(EMXXFLAGS) -c -o $@
# link object files to binary
LINK.o = $(LD) $(LDFLAGS) $(EMLDFLAGS) $(LDLIBS) -o $@
# postcompile step

all: $(OUTWEB)

.PHONY: clean
clean:
	rm $(OUTWEB) -r build

.PHONY: help
help:
	@echo available targets: all clean

$(OUTWEB): $(OBJS)
	$(LINK.o) $^

$(OBJDIR)/%.o: %.cpp
$(OBJDIR)/%.o: %.cpp $(DEPDIR)/%.d
	$(COMPILE.cc) $<

.PRECIOUS = $(DEPDIR)/%.d
$(DEPDIR)/%.d: ;

-include $(DEPS)
