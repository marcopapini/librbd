# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/amd64/avx512f/bridge_amd64_avx512f.c \
../source/amd64/avx512f/cold_standby_amd64_avx512f.c \
../source/amd64/avx512f/failure_density_amd64_avx512f.c \
../source/amd64/avx512f/hot_standby_amd64_avx512f.c \
../source/amd64/avx512f/koon_amd64_avx512f.c \
../source/amd64/avx512f/integral_amd64_avx512f.c \
../source/amd64/avx512f/parallel_amd64_avx512f.c \
../source/amd64/avx512f/series_amd64_avx512f.c 

C_DEPS += \
./source/amd64/avx512f/bridge_amd64_avx512f.d \
./source/amd64/avx512f/cold_standby_amd64_avx512f.d \
./source/amd64/avx512f/failure_density_amd64_avx512f.d \
./source/amd64/avx512f/hot_standby_amd64_avx512f.d \
./source/amd64/avx512f/koon_amd64_avx512f.d \
./source/amd64/avx512f/integral_amd64_avx512f.d \
./source/amd64/avx512f/parallel_amd64_avx512f.d \
./source/amd64/avx512f/series_amd64_avx512f.d 

OBJS_AR += \
./source/amd64/avx512f/bridge_amd64_avx512f.ar.o \
./source/amd64/avx512f/cold_standby_amd64_avx512f.ar.o \
./source/amd64/avx512f/failure_density_amd64_avx512f.ar.o \
./source/amd64/avx512f/hot_standby_amd64_avx512f.ar.o \
./source/amd64/avx512f/koon_amd64_avx512f.ar.o \
./source/amd64/avx512f/integral_amd64_avx512f.ar.o \
./source/amd64/avx512f/parallel_amd64_avx512f.ar.o \
./source/amd64/avx512f/series_amd64_avx512f.ar.o 

OBJS_SO += \
./source/amd64/avx512f/bridge_amd64_avx512f.so.o \
./source/amd64/avx512f/cold_standby_amd64_avx512f.so.o \
./source/amd64/avx512f/failure_density_amd64_avx512f.so.o \
./source/amd64/avx512f/hot_standby_amd64_avx512f.so.o \
./source/amd64/avx512f/koon_amd64_avx512f.so.o \
./source/amd64/avx512f/integral_amd64_avx512f.so.o \
./source/amd64/avx512f/parallel_amd64_avx512f.so.o \
./source/amd64/avx512f/series_amd64_avx512f.so.o 

./source/amd64/avx512f/integral_amd64_avx512f.ar.o: override C_FLAGS += $(C_FLAGS_STRICT_MATH)
./source/amd64/avx512f/integral_amd64_avx512f.so.o: override C_FLAGS += $(C_FLAGS_STRICT_MATH)


# Each subdirectory must supply rules for building sources it contributes
source/amd64/avx512f/%.ar.o: ../source/amd64/avx512f/%.c source/amd64/avx512f/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/amd64/avx512f/%.so.o: ../source/amd64/avx512f/%.c source/amd64/avx512f/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

