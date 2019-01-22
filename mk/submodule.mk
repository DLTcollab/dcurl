# libtuv related variables
LIBTUV_PATH = deps/libtuv
LIBTUV_INCLUDE := -I $(LIBTUV_PATH)/include
# PIC (Position-Independent-Code) library
LIBTUV_LIBRARY := $(LIBTUV_PATH)/build/x86_64-linux/release/lib/libtuv.o

$(LIBTUV_LIBRARY):
	git submodule update --init $(LIBTUV_PATH)
	$(MAKE) -C $(LIBTUV_PATH) TUV_BUILD_TYPE=release TUV_CREATE_PIC_LIB=yes
