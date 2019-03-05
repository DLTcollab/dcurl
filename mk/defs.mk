# Debug, debug + Sanitizer or release build
# Acceptable values
# 0: release build
# 1: debug build
# address: debug build + address Sanitizer
# undefined: debug build + undefined behavior Sanitizer
# thread: debug build + thread Sanitizer
BUILD_DEBUG ?= 0

# Build AVX backend or not
BUILD_AVX ?= 0

# Build SSE backend or not
BUILD_SSE ?= 0

# Build OpenCL backend or not
BUILD_GPU ?= 0

# Build FPGA backend or not
BUILD_FPGA_ACCEL ?= 0

# Build JNI glue as the bridge between dcurl and IRI
BUILD_JNI ?= 0

# Build cCurl compatible interface
BUILD_COMPAT ?= 0

# Show the PoW-related statistic messages or not
BUILD_STAT ?= 0
