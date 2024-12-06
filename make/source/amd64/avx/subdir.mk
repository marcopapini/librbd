# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/amd64/avx/bridge_amd64_avx.c \
../source/amd64/avx/koon_amd64_avx.c \
../source/amd64/avx/parallel_amd64_avx.c \
../source/amd64/avx/series_amd64_avx.c 

C_DEPS += \
./source/amd64/avx/bridge_amd64_avx.d \
./source/amd64/avx/koon_amd64_avx.d \
./source/amd64/avx/parallel_amd64_avx.d \
./source/amd64/avx/series_amd64_avx.d 

OBJS_AR += \
./source/amd64/avx/bridge_amd64_avx.ar.o \
./source/amd64/avx/koon_amd64_avx.ar.o \
./source/amd64/avx/parallel_amd64_avx.ar.o \
./source/amd64/avx/series_amd64_avx.ar.o 

OBJS_SO += \
./source/amd64/avx/bridge_amd64_avx.so.o \
./source/amd64/avx/koon_amd64_avx.so.o \
./source/amd64/avx/parallel_amd64_avx.so.o \
./source/amd64/avx/series_amd64_avx.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/amd64/avx/%.ar.o: ../source/amd64/avx/%.c source/amd64/avx/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/amd64/avx/%.so.o: ../source/amd64/avx/%.c source/amd64/avx/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

