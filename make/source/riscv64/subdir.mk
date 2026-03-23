# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/riscv64/bridge_riscv64.c \
../source/riscv64/koon_riscv64.c \
../source/riscv64/parallel_riscv64.c \
../source/riscv64/rbd_internal_riscv64.c \
../source/riscv64/series_riscv64.c 

C_DEPS += \
./source/riscv64/bridge_riscv64.d \
./source/riscv64/koon_riscv64.d \
./source/riscv64/parallel_riscv64.d \
./source/riscv64/rbd_internal_riscv64.d \
./source/riscv64/series_riscv64.d 

OBJS_AR += \
./source/riscv64/bridge_riscv64.ar.o \
./source/riscv64/koon_riscv64.ar.o \
./source/riscv64/parallel_riscv64.ar.o \
./source/riscv64/rbd_internal_riscv64.ar.o \
./source/riscv64/series_riscv64.ar.o 

OBJS_SO += \
./source/riscv64/bridge_riscv64.so.o \
./source/riscv64/koon_riscv64.so.o \
./source/riscv64/parallel_riscv64.so.o \
./source/riscv64/rbd_internal_riscv64.so.o \
./source/riscv64/series_riscv64.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/riscv64/%.ar.o: ../source/riscv64/%.c source/riscv64/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/riscv64/%.so.o: ../source/riscv64/%.c source/riscv64/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

