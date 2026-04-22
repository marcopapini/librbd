# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/riscv64/rvv/bridge_riscv64_rvv.c \
../source/riscv64/rvv/koon_riscv64_rvv.c \
../source/riscv64/rvv/parallel_riscv64_rvv.c \
../source/riscv64/rvv/processor_riscv64_rvv.c \
../source/riscv64/rvv/series_riscv64_rvv.c 

C_DEPS += \
./source/riscv64/rvv/bridge_riscv64_rvv.d \
./source/riscv64/rvv/koon_riscv64_rvv.d \
./source/riscv64/rvv/parallel_riscv64_rvv.d \
./source/riscv64/rvv/processor_riscv64_rvv.d \
./source/riscv64/rvv/series_riscv64_rvv.d 

OBJS_AR += \
./source/riscv64/rvv/bridge_riscv64_rvv.ar.o \
./source/riscv64/rvv/koon_riscv64_rvv.ar.o \
./source/riscv64/rvv/parallel_riscv64_rvv.ar.o \
./source/riscv64/rvv/processor_riscv64_rvv.ar.o \
./source/riscv64/rvv/series_riscv64_rvv.ar.o 

OBJS_SO += \
./source/riscv64/rvv/bridge_riscv64_rvv.so.o \
./source/riscv64/rvv/koon_riscv64_rvv.so.o \
./source/riscv64/rvv/parallel_riscv64_rvv.so.o \
./source/riscv64/rvv/processor_riscv64_rvv.so.o \
./source/riscv64/rvv/series_riscv64_rvv.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/riscv64/rvv/%.ar.o: ../source/riscv64/rvv/%.c source/riscv64/rvv/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/riscv64/rvv/%.so.o: ../source/riscv64/rvv/%.c source/riscv64/rvv/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

