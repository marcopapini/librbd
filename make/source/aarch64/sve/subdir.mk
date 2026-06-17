# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/aarch64/sve/bridge_aarch64_sve.c \
../source/aarch64/sve/cold_standby_aarch64_sve.c \
../source/aarch64/sve/failure_density_aarch64_sve.c \
../source/aarch64/sve/hot_standby_aarch64_sve.c \
../source/aarch64/sve/koon_aarch64_sve.c \
../source/aarch64/sve/integral_aarch64_sve.c \
../source/aarch64/sve/parallel_aarch64_sve.c \
../source/aarch64/sve/processor_aarch64_sve.c \
../source/aarch64/sve/series_aarch64_sve.c 

C_DEPS += \
./source/aarch64/sve/bridge_aarch64_sve.d \
./source/aarch64/sve/cold_standby_aarch64_sve.d \
./source/aarch64/sve/failure_density_aarch64_sve.d \
./source/aarch64/sve/hot_standby_aarch64_sve.d \
./source/aarch64/sve/koon_aarch64_sve.d \
./source/aarch64/sve/integral_aarch64_sve.d \
./source/aarch64/sve/parallel_aarch64_sve.d \
./source/aarch64/sve/processor_aarch64_sve.d \
./source/aarch64/sve/series_aarch64_sve.d 

OBJS_AR += \
./source/aarch64/sve/bridge_aarch64_sve.ar.o \
./source/aarch64/sve/cold_standby_aarch64_sve.ar.o \
./source/aarch64/sve/failure_density_aarch64_sve.ar.o \
./source/aarch64/sve/hot_standby_aarch64_sve.ar.o \
./source/aarch64/sve/koon_aarch64_sve.ar.o \
./source/aarch64/sve/integral_aarch64_sve.ar.o \
./source/aarch64/sve/parallel_aarch64_sve.ar.o \
./source/aarch64/sve/processor_aarch64_sve.ar.o \
./source/aarch64/sve/series_aarch64_sve.ar.o 

OBJS_SO += \
./source/aarch64/sve/bridge_aarch64_sve.so.o \
./source/aarch64/sve/cold_standby_aarch64_sve.so.o \
./source/aarch64/sve/failure_density_aarch64_sve.so.o \
./source/aarch64/sve/hot_standby_aarch64_sve.so.o \
./source/aarch64/sve/koon_aarch64_sve.so.o \
./source/aarch64/sve/integral_aarch64_sve.so.o \
./source/aarch64/sve/parallel_aarch64_sve.so.o \
./source/aarch64/sve/processor_aarch64_sve.so.o \
./source/aarch64/sve/series_aarch64_sve.so.o 

./source/aarch64/sve/integral_aarch64_sve.ar.o: override C_FLAGS += $(C_FLAGS_STRICT_MATH)
./source/aarch64/sve/integral_aarch64_sve.so.o: override C_FLAGS += $(C_FLAGS_STRICT_MATH)


# Each subdirectory must supply rules for building sources it contributes
source/aarch64/sve/%.ar.o: ../source/aarch64/sve/%.c source/aarch64/sve/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/aarch64/sve/%.so.o: ../source/aarch64/sve/%.c source/aarch64/sve/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

