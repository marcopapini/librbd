# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/bridge.c \
../source/koon.c \
../source/parallel.c \
../source/series.c 

C_DEPS += \
./source/bridge.d \
./source/koon.d \
./source/parallel.d \
./source/series.d 

OBJS_AR += \
./source/bridge.ar.o \
./source/koon.ar.o \
./source/parallel.ar.o \
./source/series.ar.o 

OBJS_SO += \
./source/bridge.so.o \
./source/koon.so.o \
./source/parallel.so.o \
./source/series.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/%.ar.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/%.so.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

