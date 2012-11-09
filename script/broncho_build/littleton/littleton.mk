PRODUCT_PACKAGES := \
	SoundRecorder \
	VideoPlayer		\
	Stk \
	PinyinIME \
	HandwritingIME \
	Flashlight \
	flashlightwidget \
	Development

$(call inherit-product, build/target/product/generic.mk)

PRODUCT_BRAND := marvell
PRODUCT_NAME := littleton
PRODUCT_DEVICE := littleton
