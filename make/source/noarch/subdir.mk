# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/noarch/bridge_noarch.c \
../source/noarch/cold_standby_noarch.c \
../source/noarch/failure_density_noarch.c \
../source/noarch/hot_standby_noarch.c \
../source/noarch/koon_noarch.c \
../source/noarch/integral_noarch.c \
../source/noarch/parallel_noarch.c \
../source/noarch/rbd_internal_noarch.c \
../source/noarch/reliability_noarch.c \
../source/noarch/series_noarch.c 

C_DEPS += \
./source/noarch/bridge_noarch.d \
./source/noarch/cold_standby_noarch.d \
./source/noarch/failure_density_noarch.d \
./source/noarch/hot_standby_noarch.d \
./source/noarch/koon_noarch.d \
./source/noarch/integral_noarch.d \
./source/noarch/parallel_noarch.d \
./source/noarch/rbd_internal_noarch.d \
./source/noarch/reliability_noarch.d \
./source/noarch/series_noarch.d 

OBJS_AR += \
./source/noarch/bridge_noarch.ar.o \
./source/noarch/cold_standby_noarch.ar.o \
./source/noarch/failure_density_noarch.ar.o \
./source/noarch/hot_standby_noarch.ar.o \
./source/noarch/koon_noarch.ar.o \
./source/noarch/integral_noarch.ar.o \
./source/noarch/parallel_noarch.ar.o \
./source/noarch/rbd_internal_noarch.ar.o \
./source/noarch/reliability_noarch.ar.o \
./source/noarch/series_noarch.ar.o 

OBJS_SO += \
./source/noarch/bridge_noarch.so.o \
./source/noarch/cold_standby_noarch.so.o \
./source/noarch/failure_density_noarch.so.o \
./source/noarch/hot_standby_noarch.so.o \
./source/noarch/koon_noarch.so.o \
./source/noarch/integral_noarch.so.o \
./source/noarch/parallel_noarch.so.o \
./source/noarch/rbd_internal_noarch.so.o \
./source/noarch/reliability_noarch.so.o \
./source/noarch/series_noarch.so.o

./source/noarch/integral_noarch.ar.o: override C_FLAGS += $(C_FLAGS_STRICT_MATH)
./source/noarch/integral_noarch.so.o: override C_FLAGS += $(C_FLAGS_STRICT_MATH)


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

