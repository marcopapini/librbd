# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/x86/bridge_x86.c \
../source/x86/koon_x86.c \
../source/x86/parallel_x86.c \
../source/x86/processor_x86.c \
../source/x86/rbd_internal_x86.c \
../source/x86/series_x86.c 

C_DEPS += \
./source/x86/bridge_x86.d \
./source/x86/koon_x86.d \
./source/x86/parallel_x86.d \
./source/x86/processor_x86.d \
./source/x86/rbd_internal_x86.d \
./source/x86/series_x86.d 

OBJS_AR += \
./source/x86/bridge_x86.ar.o \
./source/x86/koon_x86.ar.o \
./source/x86/parallel_x86.ar.o \
./source/x86/processor_x86.ar.o \
./source/x86/rbd_internal_x86.ar.o \
./source/x86/series_x86.ar.o 

OBJS_SO += \
./source/x86/bridge_x86.so.o \
./source/x86/koon_x86.so.o \
./source/x86/parallel_x86.so.o \
./source/x86/processor_x86.so.o \
./source/x86/rbd_internal_x86.so.o \
./source/x86/series_x86.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/x86/%.ar.o: ../source/x86/%.c source/x86/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/x86/%.so.o: ../source/x86/%.c source/x86/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

