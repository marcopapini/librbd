# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/power8/vsx/bridge_power8_vsx.c \
../source/power8/vsx/koon_power8_vsx.c \
../source/power8/vsx/parallel_power8_vsx.c \
../source/power8/vsx/series_power8_vsx.c 

C_DEPS += \
./source/power8/vsx/bridge_power8_vsx.d \
./source/power8/vsx/koon_power8_vsx.d \
./source/power8/vsx/parallel_power8_vsx.d \
./source/power8/vsx/series_power8_vsx.d 

OBJS_AR += \
./source/power8/vsx/bridge_power8_vsx.ar.o \
./source/power8/vsx/koon_power8_vsx.ar.o \
./source/power8/vsx/parallel_power8_vsx.ar.o \
./source/power8/vsx/series_power8_vsx.ar.o 

OBJS_SO += \
./source/power8/vsx/bridge_power8_vsx.so.o \
./source/power8/vsx/koon_power8_vsx.so.o \
./source/power8/vsx/parallel_power8_vsx.so.o \
./source/power8/vsx/series_power8_vsx.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/power8/vsx/%.ar.o: ../source/power8/vsx/%.c source/power8/vsx/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/power8/vsx/%.so.o: ../source/power8/vsx/%.c source/power8/vsx/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

