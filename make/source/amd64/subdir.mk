# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/amd64/bridge_amd64.c \
../source/amd64/koon_amd64.c \
../source/amd64/parallel_amd64.c \
../source/amd64/processor_amd64.c \
../source/amd64/rbd_internal_amd64.c \
../source/amd64/series_amd64.c 

C_DEPS += \
./source/amd64/bridge_amd64.d \
./source/amd64/koon_amd64.d \
./source/amd64/parallel_amd64.d \
./source/amd64/processor_amd64.d \
./source/amd64/rbd_internal_amd64.d \
./source/amd64/series_amd64.d 

OBJS_AR += \
./source/amd64/bridge_amd64.ar.o \
./source/amd64/koon_amd64.ar.o \
./source/amd64/parallel_amd64.ar.o \
./source/amd64/processor_amd64.ar.o \
./source/amd64/rbd_internal_amd64.ar.o \
./source/amd64/series_amd64.ar.o 

OBJS_SO += \
./source/amd64/bridge_amd64.so.o \
./source/amd64/koon_amd64.so.o \
./source/amd64/parallel_amd64.so.o \
./source/amd64/processor_amd64.so.o \
./source/amd64/rbd_internal_amd64.so.o \
./source/amd64/series_amd64.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/amd64/%.ar.o: ../source/amd64/%.c source/amd64/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/amd64/%.so.o: ../source/amd64/%.c source/amd64/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

