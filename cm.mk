# Release name
PRODUCT_RELEASE_NAME := lt02wifi

# Boot animation
TARGET_SCREEN_HEIGHT := 1280
TARGET_SCREEN_WIDTH := 800

# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_tablet_wifionly.mk)

# Enhanced NFC
$(call inherit-product, vendor/cm/config/nfc_enhanced.mk)

# Inherit device configuration
$(call inherit-product, device/samsung/lt02wifi/full_lt02wifi.mk)

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := lt02wifi
PRODUCT_NAME := cm_lt02wifi
PRODUCT_BRAND := Samsung
PRODUCT_MODEL := SM-T210R
PRODUCT_MANUFACTURER := samsung

# Set build fingerprint / ID / Product Name etc.
PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_NAME= lt02wifi \
    BUILD_FINGERPRINT="samsung/lt02wifiue/lt02wifi:4.1.2/JZO54K/T210RUEAMG4:user/release-keys" \
    PRIVATE_BUILD_DESC="lt02wifiue-user 4.1.2 JZO54K T210RUEAMG4 release-keys"
