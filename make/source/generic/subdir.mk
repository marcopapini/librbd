# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/generic/binomial.c \
../source/generic/bridge_generic.c \
../source/generic/combinations.c \
../source/generic/koon_generic.c \
../source/generic/parallel_generic.c \
../source/generic/processor_generic.c \
../source/generic/rbd_internal_generic.c \
../source/generic/series_generic.c 

C_DEPS += \
./source/generic/binomial.d \
./source/generic/bridge_generic.d \
./source/generic/combinations.d \
./source/generic/koon_generic.d \
./source/generic/parallel_generic.d \
./source/generic/processor_generic.d \
./source/generic/rbd_internal_generic.d \
./source/generic/series_generic.d 

OBJS_AR += \
./source/generic/binomial.ar.o \
./source/generic/bridge_generic.ar.o \
./source/generic/combinations.ar.o \
./source/generic/koon_generic.ar.o \
./source/generic/parallel_generic.ar.o \
./source/generic/processor_generic.ar.o \
./source/generic/rbd_internal_generic.ar.o \
./source/generic/series_generic.ar.o 

OBJS_SO += \
./source/generic/binomial.so.o \
./source/generic/bridge_generic.so.o \
./source/generic/combinations.so.o \
./source/generic/koon_generic.so.o \
./source/generic/parallel_generic.so.o \
./source/generic/processor_generic.so.o \
./source/generic/rbd_internal_generic.so.o \
./source/generic/series_generic.so.o 


# Each subdirectory must supply rules for building sources it contributes
source/generic/%.ar.o: ../source/generic/%.c source/generic/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/generic/%.so.o: ../source/generic/%.c source/generic/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

