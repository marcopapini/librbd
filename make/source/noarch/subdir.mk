# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/noarch/bridge_noarch.c \
../source/noarch/koon_noarch.c \
../source/noarch/parallel_noarch.c \
../source/noarch/rbd_internal_noarch.c \
../source/noarch/series_noarch.c 

C_DEPS += \
./source/noarch/bridge_noarch.d \
./source/noarch/koon_noarch.d \
./source/noarch/parallel_noarch.d \
./source/noarch/rbd_internal_noarch.d \
./source/noarch/series_noarch.d 

OBJS_AR += \
./source/noarch/bridge_noarch.ar.o \
./source/noarch/koon_noarch.ar.o \
./source/noarch/parallel_noarch.ar.o \
./source/noarch/rbd_internal_noarch.ar.o \
./source/noarch/series_noarch.ar.o 

OBJS_SO += \
./source/noarch/bridge_noarch.so.o \
./source/noarch/koon_noarch.so.o \
./source/noarch/parallel_noarch.so.o \
./source/noarch/rbd_internal_noarch.so.o \
./source/noarch/series_noarch.so.o


# Each subdirectory must supply rules for building sources it contributes
source/%.ar.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.ar.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

source/%.so.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: C Compiler'
	$(CC) $(C_FLAGS) $(C_FLAGS_SHARED) -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.so.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

