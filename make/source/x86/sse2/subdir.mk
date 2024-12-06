# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/x86/sse2/bridge_x86_sse2.c \
../source/x86/sse2/koon_x86_sse2.c \
../source/x86/sse2/parallel_x86_sse2.c \
../source/x86/sse2/series_x86_sse2.c 

C_DEPS += \
./source/x86/sse2/bridge_x86_sse2.d \
./source/x86/sse2/koon_x86_sse2.d \
./source/x86/sse2/parallel_x86_sse2.d \
./source/x86/sse2/series_x86_sse2.d 

OBJS_AR += \
./source/x86/sse2/bridge_x86_sse2.ar.o \
./source/x86/sse2/koon_x86_sse2.ar.o \
./source/x86/sse2/parallel_x86_sse2.ar.o \
./source/x86/sse2/series_x86_sse2.ar.o 

OBJS_SO += \
./source/x86/sse2/bridge_x86_sse2.so.o \
./source/x86/sse2/koon_x86_sse2.so.o \
./source/x86/sse2/parallel_x86_sse2.so.o \
./source/x86/sse2/series_x86_sse2.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/x86/sse2/%.ar.o: ../source/x86/sse2/%.c source/x86/sse2/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/x86/sse2/%.so.o: ../source/x86/sse2/%.c source/x86/sse2/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

