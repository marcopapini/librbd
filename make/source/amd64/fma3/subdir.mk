# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/amd64/fma3/bridge_amd64_fma3.c \
../source/amd64/fma3/koon_amd64_fma3.c \
../source/amd64/fma3/parallel_amd64_fma3.c 

C_DEPS += \
./source/amd64/fma3/bridge_amd64_fma3.d \
./source/amd64/fma3/koon_amd64_fma3.d \
./source/amd64/fma3/parallel_amd64_fma3.d 

OBJS_AR += \
./source/amd64/fma3/bridge_amd64_fma3.ar.o \
./source/amd64/fma3/koon_amd64_fma3.ar.o \
./source/amd64/fma3/parallel_amd64_fma3.ar.o 

OBJS_SO += \
./source/amd64/fma3/bridge_amd64_fma3.so.o \
./source/amd64/fma3/koon_amd64_fma3.so.o \
./source/amd64/fma3/parallel_amd64_fma3.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/amd64/fma3/%.ar.o: ../source/amd64/fma3/%.c source/amd64/fma3/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/amd64/fma3/%.so.o: ../source/amd64/fma3/%.c source/amd64/fma3/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

