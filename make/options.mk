# Provide CPU_SMP definition: set it to 1 to enable SMP, set it to 0 to disable it
C_FLAGS += -DCPU_SMP=0
# Provide CPU_ENABLE_SIMD definition: set it to 1 to enable SIMD, set it to 0 to disable it
C_FLAGS += -DCPU_ENABLE_SIMD=0

# Provide set of options needed to generate Shared Object.
# The option -fPIC is mandatory to generate Program Independent Code
C_FLAGS_SHARED += -fPIC

