# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/power8/bridge_power8.c \
../source/power8/koon_power8.c \
../source/power8/parallel_power8.c \
../source/power8/rbd_internal_power8.c \
../source/power8/series_power8.c 

C_DEPS += \
./source/power8/bridge_power8.d \
./source/power8/koon_power8.d \
./source/power8/parallel_power8.d \
./source/power8/rbd_internal_power8.d \
./source/power8/series_power8.d 

OBJS_AR += \
./source/power8/bridge_power8.ar.o \
./source/power8/koon_power8.ar.o \
./source/power8/parallel_power8.ar.o \
./source/power8/rbd_internal_power8.ar.o \
./source/power8/series_power8.ar.o 

OBJS_SO += \
./source/power8/bridge_power8.so.o \
./source/power8/koon_power8.so.o \
./source/power8/parallel_power8.so.o \
./source/power8/rbd_internal_power8.so.o \
./source/power8/series_power8.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/power8/%.ar.o: ../source/power8/%.c source/power8/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/power8/%.so.o: ../source/power8/%.c source/power8/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

