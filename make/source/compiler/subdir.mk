# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/compiler/clang.c \
../source/compiler/gcc.c \
../source/compiler/vs.c \
../source/compiler/xlc.c

C_DEPS += \
./source/compiler/clang.d \
./source/compiler/gcc.d \
./source/compiler/vs.d \
./source/compiler/xlc.d

OBJS_AR += \
./source/compiler/clang.ar.o \
./source/compiler/gcc.ar.o \
./source/compiler/vs.ar.o \
./source/compiler/xlc.ar.o

OBJS_SO += \
./source/compiler/clang.so.o \
./source/compiler/gcc.so.o \
./source/compiler/vs.so.o \
./source/compiler/xlc.so.o


# Each subdirectory must supply rules for building sources it contributes
source/compiler/%.ar.o: ../source/compiler/%.c source/compiler/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/compiler/%.so.o: ../source/compiler/%.c source/compiler/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

