# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/aarch64/bridge_aarch64.c \
../source/aarch64/koon_aarch64.c \
../source/aarch64/parallel_aarch64.c \
../source/aarch64/rbd_internal_aarch64.c \
../source/aarch64/series_aarch64.c 

C_DEPS += \
./source/aarch64/bridge_aarch64.d \
./source/aarch64/koon_aarch64.d \
./source/aarch64/parallel_aarch64.d \
./source/aarch64/rbd_internal_aarch64.d \
./source/aarch64/series_aarch64.d 

OBJS_AR += \
./source/aarch64/bridge_aarch64.ar.o \
./source/aarch64/koon_aarch64.ar.o \
./source/aarch64/parallel_aarch64.ar.o \
./source/aarch64/rbd_internal_aarch64.ar.o \
./source/aarch64/series_aarch64.ar.o 

OBJS_SO += \
./source/aarch64/bridge_aarch64.so.o \
./source/aarch64/koon_aarch64.so.o \
./source/aarch64/parallel_aarch64.so.o \
./source/aarch64/rbd_internal_aarch64.so.o \
./source/aarch64/series_aarch64.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/aarch64/%.ar.o: ../source/aarch64/%.c source/aarch64/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/aarch64/%.so.o: ../source/aarch64/%.c source/aarch64/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

