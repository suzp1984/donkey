LOCAL_PATH := $(call my-dir)

boot_nontrust_133mhz_ddr := $(PRODUCT_OUT)/boot_nontrust_133mhz_ddr.bin
$(boot_nontrust_133mhz_ddr) : $(LOCAL_PATH)/boot_nontrust_133mhz_ddr.bin | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(boot_nontrust_133mhz_ddr)

boot_nontrust_156mhz_ddr := $(PRODUCT_OUT)/boot_nontrust_156mhz_ddr.bin
$(boot_nontrust_156mhz_ddr) : $(LOCAL_PATH)/boot_nontrust_156mhz_ddr.bin | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(boot_nontrust_156mhz_ddr)

