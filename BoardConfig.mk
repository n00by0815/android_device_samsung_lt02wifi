# Copyright (C) 2013 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


-include vendor/samsung/lt02wifi/BoardConfigVendor.mk

TARGET_RECOVERY_FSTAB = device/samsung/lt02wifi/fstab.
.
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR ?= device/samsung/lt02wifi/bluetooth

TARGET_BOARD_PLATFORM := mrvl

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := cortex-a9

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_BOOTIMAGE_PARTITION_SIZE := 12582912
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 12582912
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1507852288
BOARD_USERDATAIMAGE_PARTITION_SIZE := 5775556608
BOARD_FLASH_BLOCK_SIZE := 4096


