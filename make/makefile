-include ../makefile.init

RM := rm -rf

-include tools.mk
-include options.mk

# All of the sources participating in the build are defined here
-include sources.mk
-include source/subdir.mk
-include source/aarch64/subdir.mk
-include source/aarch64/neon/subdir.mk
-include source/amd64/subdir.mk
-include source/amd64/avx/subdir.mk
-include source/amd64/avx512f/subdir.mk
-include source/amd64/fma3/subdir.mk
-include source/compiler/subdir.mk
-include source/generic/subdir.mk
-include source/noarch/subdir.mk
-include source/os/subdir.mk
-include source/x86/subdir.mk
-include source/x86/sse2/subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := rbd
BUILD_ARTIFACT_EXTENSION_ARCHIVE := a
BUILD_ARTIFACT_EXTENSION_SHARED := so
BUILD_ARTIFACT_PREFIX := lib
BUILD_ARTIFACT_ARCHIVE := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION_ARCHIVE),.$(BUILD_ARTIFACT_EXTENSION_ARCHIVE),)
BUILD_ARTIFACT_SHARED := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION_SHARED),.$(BUILD_ARTIFACT_EXTENSION_SHARED),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: archive shared

# Tool invocations
archive: $(OBJS_AR) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking Archiver'
	$(AR) -r $(BUILD_ARTIFACT_ARCHIVE) $(OBJS_AR) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	
shared: $(OBJS_SO) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking C Linker'
	$(LD) -shared -Wl,-soname=$(BUILD_ARTIFACT_NAME) -o $(BUILD_ARTIFACT_SHARED) $(OBJS_SO) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
cleanall: clean-archive clean-shared
	-$(RM) $(C_DEPS)
	-@echo ' '
	
clean-archive:
	-$(RM) $(BUILD_ARTIFACT_ARCHIVE)
	-$(RM) $(OBJS_AR)
	-@echo ' '
	
clean-shared:
	-$(RM) $(BUILD_ARTIFACT_SHARED)
	-$(RM) $(OBJS_SO)
	-@echo ' '

.PHONY: all cleanall clean-archive clean-shared dependents main-build

-include ../makefile.targets
