#Configure flags etc.
CFLAGS :=-g
CXXFLAGS :=-g
PROGNAME :=VkEngine

SRC_LIBS := config
LIBS := glfw vulkan ply tga pthread

# Finding source files
C_FILES := $(shell find src/ -name "*.cpp" -or -name "*.cc" -or -name "*.c")# | sed ':a;N;$!ba;s/\n/ /g')
L_FILES := $(shell find src/ -name "*.l")
Y_FILES := $(shell find src/ -name "*.y")

INCLUDE_DIRS := src/ include/ $(addsuffix /,$(shell find srclibs/ -name "include"))
LIBRARY_DIRS := lib/linux_amd64/ $(addsuffix /,$(shell find srclibs/ -name "lib"))

# Toolchain-setup
CC := gcc
CXX := g++
LD := ld
AR := ar
LEX := lex
YACC := yacc
AS := as

# Functions used for target-generation
get_dependency = $(shell g++ -MM -Isrc/ ${1}| sed -e ':a;N;$$!ba;s/\\\n //g' | tee compiler_out.txt  | sed -e 's/[A-Za-z\.\/_-]*: //')
obj_target=obj/${2}/$(addsuffix .o,$(basename ${1}))
lexer_target=generated/$(basename ${1}).scanner.c
parser_target=generated/$(basename ${1}).parser.c
srclib_target=srclibs/${1}/lib/lib${1}.a

define obj
$(call obj_target,${1},${2}) : $(call get_dependency,${1}) | obj/Debug/
obj$(suffix ${1}) += $(call obj_target,${1},${2})
endef

define lexer
$(call lexer_target,${1}) : ${1} | generated/
scanner$(suffix${1})+= $(call lexer_target,${1})
C_FILES += $(call lexer_target,${1})
endef

define parser
$(call parser_target,${1}) : ${1} | generated/
parser$(suffix ${1}) += $(call parser_target,${1})
C_FILES += $(call parser_target,${1})
endef

define srclib
$(call srclib_target,${1}) : srclibs/${1}/
srclibs+=$(call srclib_target,${1})
endef

all: bin/Debug/${PROGNAME}

Debug: bin/Debug/${PROGNAME}

Library: lib/lib${PROGNAME}.a

$(foreach src,${C_FILES},$(eval $(call obj,${src},Debug)))
$(foreach lib,${SRC_LIBS},$(eval $(call srclib,${lib})))

O_FILES:=$(foreach src,${C_FILES},$(call obj_target,${src},Debug))
LIBRARY_O_FILES := $(filter-out $(call obj_target,src/main.cpp,Debug),${O_FILES})
SRC_LIB_ARCHS := $(foreach lib,${SRC_LIBS},$(call srclib_target,${lib}))

#Template targets

bin/Debug/${PROGNAME}: ${O_FILES} | bin/Debug/ ${SRC_LIB_ARCHS}
	@mkdir -p bin/Debug
	@echo ${LIBRARY_DIRS}
	$(CXX) -o $@ $^ $(addprefix -L,${LIBRARY_DIRS}) $(addprefix -l, ${LIBS}) $(addprefix -l, ${SRC_LIBS})

lib/lib${PROGNAME}.a: ${LIBRARY_O_FILES} | lib/
	@echo Creating library
	$(AR) -rcs $@ $^

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

${scanner.l} : % :
	$(LEX) -o $@ $^

${parser.y} : % :
	$(YACC) -v -d $^ -o $@

${srclibs} : % :
	cd srclibs/$(word 2,$(subst /, ,$@))/ && make Library

obj/Debug/:
	@mkdir -p obj/Debug/

bin/Debug/:
	@mkdir -p bin/Debug/

generated/:
	@mkdir -p generated/

lib/:
	@mkdir -p lib/

cleanDebug: clean

cleanRelease: clean

clean:
	@rm -r obj
	@rm -r bin
