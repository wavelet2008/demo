ROOT_DIR:=$(shell pwd)
TARGET:= libstduent.so
# TARGET:= libstduent.a

CC		:= arm-himix200-linux-g++  
AR		:= arm-himix200-linux-ar
CFLAGS	:= -mcpu=cortex-a53 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations \
-W -Wall -O3 -std=c++11 -DNNIE -fpic -fno-strict-aliasing -traditional -Wno-reorder -Wno-char-subscripts
INCLUDE	:= 
LIBS	:= 
STATIC_LIB:=
LIBS	+= $(STATIC_LIB)

SRC_DIR	+=$(ROOT_DIR)/demo
SRC_FILE_CPP+=$(foreach d, $(SRC_DIR), $(wildcard $(addprefix $(d)\/*, .cpp))) 
INCLUDE_FILE+=$(foreach d, $(SRC_DIR), $(wildcard $(addprefix $(d)\/*, .h))) 
SOURCE  +=$(SRC_FILE_CPP)

OBJS    := $(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SOURCE)))

ifndef PREFIX
	RELEASE_DIR:=${ROOT_DIR}/release
else
	RELEASE_DIR:=${PREFIX}
endif

ifeq ($(suffix $(TARGET)), .a)
	IS_LIB:=TURE
endif

ifeq ($(suffix $(TARGET)), .so)
	IS_LIB:=TURE
endif

.PHONY:all
all: $(TARGET)

$(TARGET): $(OBJS)
ifeq ($(suffix $(TARGET)), .a) 
	$(AR) -rc $@ $(OBJS) $(LIBS)
else
ifeq ($(suffix $(TARGET)), .so) 
	$(CC) $(CFLAGS) -shared -fpic -o $@ $(OBJS) $(LIBS)
else
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
endif
endif
	@echo ${TARGET}
	@echo ${RELEASE_DIR}
ifeq ($(IS_LIB), TURE) 
	@mkdir -p ${RELEASE_DIR}/lib
	@cp -f $(TARGET) ${RELEASE_DIR}/lib
	@mkdir -p ${RELEASE_DIR}/include
	@for include_file in $(INCLUDE_FILE); do \
		echo $$include_file; \
		cp -f $$include_file ${RELEASE_DIR}/include; \
	done
else
	@mkdir -p ${RELEASE_DIR}/bin
	@cp -f $(TARGET) ${RELEASE_DIR}/bin
endif	

%.o : %.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

.PHONY:clean
clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
	rm -rf ${RELEASE_DIR}/bin
	rm -rf ${RELEASE_DIR}/lib
	rm -rf ${RELEASE_DIR}/include
	