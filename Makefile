# we assume that the utilities from RISC-V cross-compiler (i.e., riscv64-unknown-elf-gcc and etc.)
# are in your system PATH. To check if your environment satisfies this requirement, simple use 
# `which` command as follows:
# $ which riscv64-unknown-elf-gcc
# if you have an output path, your environment satisfy our requirement.

# ---------------------	macros --------------------------
CROSS_PREFIX 	:= riscv64-unknown-elf-
CC 				:= $(CROSS_PREFIX)gcc
AR 				:= $(CROSS_PREFIX)ar
RANLIB        	:= $(CROSS_PREFIX)ranlib

SRC_DIR        	:= .
OBJ_DIR 		:= obj
SPROJS_INCLUDE 	:= -I.  

HOSTFS_ROOT := hostfs_root
ifneq (,)
  march := -march=
  is_32bit := $(findstring 32,$(march))
  mabi := -mabi=$(if $(is_32bit),ilp32,lp64)
endif
CFLAGS        := -Wall -Werror -gdwarf-3 -fno-builtin -nostdlib -D__NO_INLINE__ -mcmodel=medany -g -Og -std=gnu99 -Wno-unused -Wno-attributes -fno-delete-null-pointer-checks -fno-PIE $(march)
#CFLAGS        := -Wall -Werror  -fno-builtin -nostdlib -D__NO_INLINE__ -mcmodel=medany -g -Og -std=gnu99 -Wno-unused -Wno-attributes -fno-delete-null-pointer-checks -fno-PIE $(march)
COMPILE       	:= $(CC) -MMD -MP $(CFLAGS) $(SPROJS_INCLUDE)

#---------------------	utils -----------------------
UTIL_CPPS 	:= util/*.c

UTIL_CPPS  := $(wildcard $(UTIL_CPPS))
UTIL_OBJS  :=  $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(UTIL_CPPS)))


UTIL_LIB   := $(OBJ_DIR)/util.a

#---------------------	kernel -----------------------
KERNEL_LDS  	:= kernel/kernel.lds
KERNEL_CPPS 	:= \
	kernel/*.c \
	kernel/machine/*.c \
	kernel/util/*.c

KERNEL_ASMS 	:= \
	kernel/*.S \
	kernel/machine/*.S \
	kernel/util/*.S

KERNEL_CPPS  	:= $(wildcard $(KERNEL_CPPS))
KERNEL_ASMS  	:= $(wildcard $(KERNEL_ASMS))
KERNEL_OBJS  	:=  $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(KERNEL_CPPS)))
KERNEL_OBJS  	+=  $(addprefix $(OBJ_DIR)/, $(patsubst %.S,%.o,$(KERNEL_ASMS)))

KERNEL_TARGET = $(OBJ_DIR)/riscv-pke


#---------------------	spike interface library -----------------------
SPIKE_INF_CPPS 	:= spike_interface/*.c

SPIKE_INF_CPPS  := $(wildcard $(SPIKE_INF_CPPS))
SPIKE_INF_OBJS 	:=  $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(SPIKE_INF_CPPS)))


SPIKE_INF_LIB   := $(OBJ_DIR)/spike_interface.a


#---------------------	user   -----------------------
USER_CPPS 		:= user/app_shell.c user/user_lib.c

USER_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_CPPS)))

USER_TARGET 	:= $(HOSTFS_ROOT)/bin/app_shell

USER_E_CPPS 		:= user/app_ls.c user/user_lib.c

USER_E_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_E_CPPS)))

USER_E_TARGET 	:= $(HOSTFS_ROOT)/bin/app_ls

USER_M_CPPS 		:= user/app_mkdir.c user/user_lib.c

USER_M_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_M_CPPS)))

USER_M_TARGET 	:= $(HOSTFS_ROOT)/bin/app_mkdir

USER_T_CPPS 		:= user/app_touch.c user/user_lib.c

USER_T_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_T_CPPS)))

USER_T_TARGET 	:= $(HOSTFS_ROOT)/bin/app_touch

USER_C_CPPS 		:= user/app_cat.c user/user_lib.c

USER_C_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_C_CPPS)))

USER_C_TARGET 	:= $(HOSTFS_ROOT)/bin/app_cat

USER_O_CPPS 		:= user/app_echo.c user/user_lib.c

USER_O_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_O_CPPS)))

USER_O_TARGET 	:= $(HOSTFS_ROOT)/bin/app_echo

USER_B_CPPS 		:= user/app_backtrace.c user/user_lib.c

USER_B_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_B_CPPS)))

USER_B_TARGET 	:= $(HOSTFS_ROOT)/bin/app_backtrace	

USER_S_CPPS 		:= user/app_sequence.c user/user_lib.c

USER_S_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_S_CPPS)))

USER_S_TARGET 	:= $(HOSTFS_ROOT)/bin/app_sequence

USER_D_CPPS 		:= user/app_wait_challengex.c user/user_lib.c

USER_D_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_D_CPPS)))

USER_D_TARGET 	:= $(HOSTFS_ROOT)/bin/app_wait_challengex

USER_F_CPPS 		:= user/app_relativepath_challengex.c user/user_lib.c

USER_F_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_F_CPPS)))

USER_F_TARGET 	:= $(HOSTFS_ROOT)/bin/app_relativepath_challengex

USER_G_CPPS 		:= user/app_errorline_challengex.c user/user_lib.c

USER_G_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_G_CPPS)))

USER_G_TARGET 	:= $(HOSTFS_ROOT)/bin/app_errorline_challengex

USER_H_CPPS 		:= user/app_singlepageheap_challengex.c user/user_lib.c

USER_H_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_H_CPPS)))

USER_H_TARGET 	:= $(HOSTFS_ROOT)/bin/app_singlepageheap_challengex

USER_I_CPPS 		:= user/app_semaphore_challengex.c user/user_lib.c

USER_I_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_I_CPPS)))

USER_I_TARGET 	:= $(HOSTFS_ROOT)/bin/app_semaphore_challengex

USER_L_CPPS 		:= user/app_exec_challengex.c user/user_lib.c

USER_L_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_L_CPPS)))

USER_L_TARGET 	:= $(HOSTFS_ROOT)/bin/app_exec_challengex

USER_P_CPPS 		:= user/app0.c user/user_lib.c

USER_P_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_P_CPPS)))

USER_P_TARGET 	:= $(HOSTFS_ROOT)/bin/app0

USER_N_CPPS 		:= user/app1.c user/user_lib.c

USER_N_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_N_CPPS)))

USER_N_TARGET 	:= $(HOSTFS_ROOT)/bin/app1

USER_A_CPPS 		:= user/app_cow.c user/user_lib.c

USER_A_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_A_CPPS)))

USER_A_TARGET 	:= $(HOSTFS_ROOT)/bin/app_cow

USER_V_CPPS 		:= user/app_shell.c user/user_lib.c

USER_V_OBJS  		:= $(addprefix $(OBJ_DIR)/, $(patsubst %.c,%.o,$(USER_V_CPPS)))

USER_V_TARGET 	:= $(HOSTFS_ROOT)/bin/app_shell



#------------------------targets------------------------
$(OBJ_DIR):
	@-mkdir -p $(OBJ_DIR)	
	@-mkdir -p $(dir $(UTIL_OBJS))
	@-mkdir -p $(dir $(SPIKE_INF_OBJS))
	@-mkdir -p $(dir $(KERNEL_OBJS))
	@-mkdir -p $(dir $(USER_OBJS))
	@-mkdir -p $(dir $(USER_E_OBJS))
	@-mkdir -p $(dir $(USER_M_OBJS))
	@-mkdir -p $(dir $(USER_T_OBJS))
	@-mkdir -p $(dir $(USER_C_OBJS))
	@-mkdir -p $(dir $(USER_O_OBJS))
	@-mkdir -p $(dir $(USER_B_OBJS))
	@-mkdir -p $(dir $(USER_S_OBJS))
	@-mkdir -p $(dir $(USER_D_OBJS))
	@-mkdir -p $(dir $(USER_F_OBJS))
	@-mkdir -p $(dir $(USER_G_OBJS))
	@-mkdir -p $(dir $(USER_H_OBJS))
	@-mkdir -p $(dir $(USER_I_OBJS))
	@-mkdir -p $(dir $(USER_L_OBJS))
	@-mkdir -p $(dir $(USER_P_OBJS))
	@-mkdir -p $(dir $(USER_N_OBJS))
	@-mkdir -p $(dir $(USER_A_OBJS))
	@-mkdir -p $(dir $(USER_V_OBJS))
	
	
	
$(OBJ_DIR)/%.o : %.c
	@echo "compiling" $<
	@$(COMPILE) -c $< -o $@

$(OBJ_DIR)/%.o : %.S
	@echo "compiling" $<
	@$(COMPILE) -c $< -o $@

$(UTIL_LIB): $(OBJ_DIR) $(UTIL_OBJS)
	@echo "linking " $@	...	
	@$(AR) -rcs $@ $(UTIL_OBJS) 
	@echo "Util lib has been build into" \"$@\"
	
$(SPIKE_INF_LIB): $(OBJ_DIR) $(UTIL_OBJS) $(SPIKE_INF_OBJS)
	@echo "linking " $@	...	
	@$(AR) -rcs $@ $(SPIKE_INF_OBJS) $(UTIL_OBJS)
	@echo "Spike lib has been build into" \"$@\"

$(KERNEL_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(SPIKE_INF_LIB) $(KERNEL_OBJS) $(KERNEL_LDS)
	@echo "linking" $@ ...
	@$(COMPILE) $(KERNEL_OBJS) $(UTIL_LIB) $(SPIKE_INF_LIB) -o $@ -T $(KERNEL_LDS)
	@echo "PKE core has been built into" \"$@\"

$(USER_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"
	@cp $@ $(OBJ_DIR)

$(USER_E_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_E_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_E_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"

$(USER_M_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_M_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_M_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"

$(USER_T_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_T_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_T_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"

$(USER_C_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_C_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_C_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"

$(USER_O_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_O_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_O_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"
	
$(USER_B_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_B_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_B_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"

$(USER_S_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_S_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_S_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"	
	
$(USER_D_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_D_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_D_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"	
	
$(USER_F_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_F_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_F_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"	
	
$(USER_G_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_G_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_G_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"	
	
$(USER_H_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_H_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_H_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"	
	
$(USER_I_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_I_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_I_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"

$(USER_L_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_L_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_L_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"

$(USER_N_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_N_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_N_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"

$(USER_P_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_P_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_P_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"

$(USER_A_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_A_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_A_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"

$(USER_V_TARGET): $(OBJ_DIR) $(UTIL_LIB) $(USER_V_OBJS)
	@echo "linking" $@	...	
	-@mkdir -p $(HOSTFS_ROOT)/bin
	@$(COMPILE) --entry=main $(USER_V_OBJS) $(UTIL_LIB) -o $@
	@echo "User app has been built into" \"$@\"



	
-include $(wildcard $(OBJ_DIR)/*/*.d)
-include $(wildcard $(OBJ_DIR)/*/*/*.d)

.DEFAULT_GOAL := $(all)

all: $(KERNEL_TARGET) $(USER_TARGET) $(USER_E_TARGET) $(USER_M_TARGET) $(USER_T_TARGET) $(USER_C_TARGET) $(USER_O_TARGET) $(USER_B_TARGET) $(USER_S_TARGET) $(USER_D_TARGET) $(USER_F_TARGET) $(USER_G_TARGET) $(USER_H_TARGET) $(USER_I_TARGET) $(USER_L_TARGET) $(USER_N_TARGET) $(USER_P_TARGET) $(USER_A_TARGET) $(USER_V_TARGET) 
.PHONY:all

run: $(KERNEL_TARGET) $(USER_TARGET) $(USER_E_TARGET) $(USER_M_TARGET) $(USER_T_TARGET) $(USER_C_TARGET) $(USER_O_TARGET) $(USER_B_TARGET) $(USER_S_TARGET) $(USER_D_TARGET) $(USER_F_TARGET) $(USER_G_TARGET) $(USER_H_TARGET) $(USER_I_TARGET) $(USER_L_TARGET) $(USER_N_TARGET) $(USER_P_TARGET) $(USER_A_TARGET) $(USER_V_TARGET) 
	@echo "********************HUST PKE********************"
	spike $(KERNEL_TARGET) /bin/app_shell

# need openocd!
gdb:$(KERNEL_TARGET) $(USER_TARGET)
	spike --rbb-port=9824 -H $(KERNEL_TARGET) $(USER_TARGET) &
	@sleep 1
	openocd -f ./.spike.cfg &
	@sleep 1
	riscv64-unknown-elf-gdb -command=./.gdbinit

# clean gdb. need openocd!
gdb_clean:
	@-kill -9 $$(lsof -i:9824 -t)
	@-kill -9 $$(lsof -i:3333 -t)
	@sleep 1

objdump:
	riscv64-unknown-elf-objdump -d $(KERNEL_TARGET) > $(OBJ_DIR)/kernel_dump
	riscv64-unknown-elf-objdump -d $(USER_TARGET) > $(OBJ_DIR)/user_dump

cscope:
	find ./ -name "*.c" > cscope.files
	find ./ -name "*.h" >> cscope.files
	find ./ -name "*.S" >> cscope.files
	find ./ -name "*.lds" >> cscope.files
	cscope -bqk

format:
	@python ./format.py ./

clean:
	rm -fr ${OBJ_DIR} ${HOSTFS_ROOT}/bin
