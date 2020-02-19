#Configure flags etc.
CFLAGS :=
CXXFLAGS :=
PROGNAME :=VkEngine

SRC_LIBS := config
LIBS := glfw vulkan ply tga pthread z BulletDynamics BulletCollision LinearMath

# Finding source files
C_FILES := $(shell find src/ -name "*.cpp" -or -name "*.cc" -or -name "*.c")# | sed ':a;N;$!ba;s/\n/ /g')
L_FILES := $(shell find src/ -name "*.l")
Y_FILES := $(shell find src/ -name "*.y")

FRAG_SHADER_FILES:=$(shell find resources/ -name "*.frag")
VERT_SHADER_FILES:=$(shell find resources/ -name "*.vert")

INCLUDE_DIRS := src/ include/ include/bullet/ $(addsuffix /,$(shell find srclibs/ -name "include"))
LIBRARY_DIRS := lib/linux_amd64/ $(addsuffix /,$(shell find srclibs/ -name "lib"))

# Toolchain-setup
CC := gcc
CXX := g++
LD := ld
AR := ar
LEX := lex
YACC := yacc
AS := as

SPIRV_COMP := ./tools/glslangValidator

# Functions used for target-generation
get_dependency = $(shell g++ -MM -Isrc/ ${1}| sed -e ':a;N;$$!ba;s/\\\n //g' | sed -e 's/[A-Za-z\.\/_-]*: //')
obj_target=obj/${2}/$(addsuffix .o,$(basename ${1}))
lexer_target=generated/$(basename ${1}).scanner.c
parser_target=generated/$(basename ${1}).parser.c
srclib_target=srclibs/${1}/lib/lib${1}.a
shader_target=${1}.spirv

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

define shader
$(call shader_target,${1}) : ${1} |
shaders.spirv+=$(call shader_target,${1})
endef

all: Debug

Debug: CFLAGS +=-g -rdynamic -DDEBUG
Debug: CXXFLAGS +=-g -rdynamic -DDEBUG
Debug: bin/Debug/${PROGNAME}

Logging: CFLAGS +=-g -rdynamic -DDEBUG -DDEBUG_LOGGING
Logging: CXXFLAGS +=-g -rdynamic -DDEBUG -DDEBUG_LOGGING
Logging: bin/Debug/${PROGNAME}

DebugW: CFLAGS +=-g -rdynamic -DDEBUG -Werror
DebugW: CXXFLAGS +=-g -rdynamic -DDEBUG -Werror
DebugW: bin/Debug/${PROGNAME}

Release: CFLAGS +=-O2
Release: CXXFLAGS += -O2
Release: bin/Release/${PROGNAME}

Library: lib/lib${PROGNAME}.a

$(foreach src,${C_FILES},$(eval $(call obj,${src},Debug)))
$(foreach lib,${SRC_LIBS},$(eval $(call srclib,${lib})))
$(foreach shdr,${VERT_SHADER_FILES},$(eval $(call shader,${shdr})))
$(foreach shdr,${FRAG_SHADER_FILES},$(eval $(call shader,${shdr})))

O_FILES:=$(foreach src,${C_FILES},$(call obj_target,${src},Debug))
LIBRARY_O_FILES := $(filter-out $(call obj_target,src/main.cpp,Debug),${O_FILES})
SRC_LIB_ARCHS := $(foreach lib,${SRC_LIBS},$(call srclib_target,${lib}))
SHADER_SPIRVS := $(foreach shdr,${VERT_SHADER_FILES},$(call shader_target,${shdr})) $(foreach shdr,${FRAG_SHADER_FILES},$(call shader_target,${shdr}))

#Template targets

bin/Debug/${PROGNAME}: ${O_FILES} | bin/Debug/ ${SRC_LIB_ARCHS} ${SHADER_SPIRVS}
	@mkdir -p bin/Debug
	@echo Linking $@
	@$(CXX) -o $@ $^ $(addprefix -L,${LIBRARY_DIRS}) $(addprefix -l, ${LIBS}) $(addprefix -l, ${SRC_LIBS}) $(CXXFLAGS)

bin/Release/${PROGNAME}: ${O_FILES} | bin/Release/ ${SRC_LIB_ARCHS} ${SHADER_SPIRVS}
	@mkdir -p bin/Debug
	@echo Linking $@
	@$(CXX) -o $@ $^ $(addprefix -L,${LIBRARY_DIRS}) $(addprefix -l, ${LIBS}) $(addprefix -l, ${SRC_LIBS}) $(CXXFLAGS)

lib/lib${PROGNAME}.a: ${LIBRARY_O_FILES} | lib/
	@echo Creating library
	@$(AR) -rcs $@ $^

${obj.c} : % :
	@echo Compiling $@
	@mkdir -p $(dir $@)
	@$(CC) -c -o $@ $< $(CFLAGS) $(addprefix -I, ${INCLUDE_DIRS})

${obj.cpp} : % :
	@echo Compiling $@
	@mkdir -p $(dir $@)
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(addprefix -I, ${INCLUDE_DIRS})

${obj.cc} : % :
	@echo Compiling $@
	@mkdir -p $(dir $@)
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(addprefix -I, ${INCLUDE_DIRS})

${scanner.l} : % :
	@echo Generating lexer $@
	@$(LEX) -o $@ $^

${parser.y} : % :
	@echo Generating parser $@
	@$(YACC) -v -d $^ -o $@

${srclibs} : % :
	cd srclibs/$(word 2,$(subst /, ,$@))/ && make Library

${shaders.spirv} : % :
	@echo Compiling shader $@
	@$(SPIRV_COMP) -V $< -o $@

obj/Debug/:
	@mkdir -p obj/Debug/

bin/Debug/:
	@mkdir -p bin/Debug/

bin/Release/:
	@mkdir -p bin/Release/

generated/:
	@mkdir -p generated/

lib/:
	@mkdir -p lib/

cleanDebug: clean

cleanRelease: clean

clean:
	@rm -r obj
	@rm -r bin
