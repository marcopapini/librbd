# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/aarch64/neon/bridge_aarch64_neon.c \
../source/aarch64/neon/koon_aarch64_neon.c \
../source/aarch64/neon/parallel_aarch64_neon.c \
../source/aarch64/neon/series_aarch64_neon.c 

C_DEPS += \
./source/aarch64/neon/bridge_aarch64_neon.d \
./source/aarch64/neon/koon_aarch64_neon.d \
./source/aarch64/neon/parallel_aarch64_neon.d \
./source/aarch64/neon/series_aarch64_neon.d 

OBJS_AR += \
./source/aarch64/neon/bridge_aarch64_neon.ar.o \
./source/aarch64/neon/koon_aarch64_neon.ar.o \
./source/aarch64/neon/parallel_aarch64_neon.ar.o \
./source/aarch64/neon/series_aarch64_neon.ar.o 

OBJS_SO += \
./source/aarch64/neon/bridge_aarch64_neon.so.o \
./source/aarch64/neon/koon_aarch64_neon.so.o \
./source/aarch64/neon/parallel_aarch64_neon.so.o \
./source/aarch64/neon/series_aarch64_neon.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/aarch64/neon/%.ar.o: ../source/aarch64/neon/%.c source/aarch64/neon/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/aarch64/neon/%.so.o: ../source/aarch64/neon/%.c source/aarch64/neon/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

