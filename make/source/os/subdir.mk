# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/os/linux.c \
../source/os/macos.c \
../source/os/unknown_os.c \
../source/os/windows.c 

C_DEPS += \
./source/os/linux.d \
./source/os/macos.d \
./source/os/unknown_os.d \
./source/os/windows.d 

OBJS_AR += \
./source/os/linux.ar.o \
./source/os/macos.ar.o \
./source/os/unknown_os.ar.o \
./source/os/windows.ar.o 

OBJS_SO += \
./source/os/linux.so.o \
./source/os/macos.so.o \
./source/os/unknown_os.so.o \
./source/os/windows.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/os/%.ar.o: ../source/os/%.c source/os/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/os/%.so.o: ../source/os/%.c source/os/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

