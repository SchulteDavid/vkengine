#Configure flags etc.
CFLAGS :=-g
CXXFLAGS :=-g
PROGNAME :=VkEngine

LIBS := glfw vulkan ply tga pthread configloader stack

# Finding source files
C_FILES := $(shell find src/ -name "*.cpp" -or -name "*.cc" -or -name "*.c")# | sed ':a;N;$!ba;s/\n/ /g')

INCLUDE_DIRS := src/ include/ $(addsuffix /,$(shell find srclibs/ -name "include"))

CC := gcc
CXX := g++
LD := ld
AR := ar
LEX := lex
YACC := yacc

get_dependency = $(shell g++ -MM -Isrc/ ${1}| sed -e ':a;N;$$!ba;s/\\\n //g' | tee compiler_out.txt  | sed -e 's/[A-Za-z\.\/_-]*: //')
obj_target := obj/${2}/$(addsuffix .o,$(basename ${1}))

define obj
obj/${2}/$(addsuffix .o,$(basename ${1})) : $(call get_dependency,${1}) | obj/Debug/
obj$(suffix ${1}) += obj/${2}/$(addsuffix .o,$(basename ${1}))
endef

all: bin/Debug/${PROGNAME}

Debug: bin/Debug/${PROGNAME}

$(foreach src,${C_FILES},$(eval $(call obj,${src},Debug)))

O_FILES:=$(foreach src,${C_FILES},obj/Debug/$(addsuffix .o,$(basename ${src})))

bin/Debug/${PROGNAME}: ${O_FILES} | bin/Debug/
	@echo ${C_FILES}
	@mkdir -p bin/Debug
	$(CXX) -o $@ $^ -Llib/linux_amd64/ $(addprefix -l, ${LIBS})

${obj.c} : % :
	@echo Compiling $@
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(addprefix -I, ${INCLUDE_DIRS})

${obj.cpp} : % :
	@echo Compiling $@
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(addprefix -I, ${INCLUDE_DIRS})

${obj.cc} : % :
	@echo Compiling $@
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(addprefix -I, ${INCLUDE_DIRS})

obj/Debug/:
	@mkdir -p obj/Debug/

bin/Debug/:
	@mkdir -p bin/Debug/

cleanDebug: clean

cleanRelease: clean

clean:
	@rm -r obj
