#---------------------------------------------------------------------------------
#
# Copyright (c) 2022-2026, SparkFun Electronics Inc.
#
# SPDX-License-Identifier: MIT
#
#---------------------------------------------------------------------------------

# Declare targets that don't represent files. This ensures make always runs them
# even if a file or directory with the same name exists in the project.
.PHONY: all build build-clean clean upload depends

#---------------------------------------------------------------------------------
# Configuration
#---------------------------------------------------------------------------------

# Directories to remove when running 'make clean'
CLEAN_DIRS := build SparkFun_DataLoggerIoT sfeDataLoggerIoT/build

# Optional flag appended to the arduino-cli command.
# Set to --clean by the build-clean target.
ARDUINO_CLEAN :=

# Upload configuration
UPLOAD_BAUD := 460800
BUILD_DIR   := ./sfeDataLoggerIoT/build/esp32.esp32.esp32

#---------------------------------------------------------------------------------
# Dependencies
#---------------------------------------------------------------------------------

# Arduino libraries to install via arduino-cli.
# Format: "LibraryName@version" or just "LibraryName" for the latest version.
ARDUINO_LIBS := \
	FastLED@3.9.15 \
	"ESP Async WebServer@3.7.7" \
	"Async TCP@3.4.2"" 

#---------------------------------------------------------------------------------
# Build Command
#---------------------------------------------------------------------------------

# Full arduino-cli compile command.
# References ARDUINO_CLEAN to optionally force a clean build.
ARDUINO_CMD := arduino-cli compile \
			--fqbn esp32:esp32:esp32 \
            ./sfeDataLoggerIoT/sfeDataLoggerIoT.ino \
            --build-property upload.maximum_size=4980736 \
            --build-property build.flash_size=16MB \
            --build-property build.partitions=partitions \
            --build-property build.flash_mode=dio \
            --build-property build.flash_freq=80m \
            --export-binaries \
			$(ARDUINO_CLEAN) \
			--library $(shell pwd)/SparkFun_DataLoggerIoT

# esptool.py flash command. PORT must be supplied by the caller.
UPLOAD_CMD := esptool.py \
	--chip esp32 \
	--port $(PORT) \
	--baud $(UPLOAD_BAUD) \
    --before default_reset \
	--after hard_reset write_flash  -z \
    --flash_mode dio \
	--flash_freq 80m \
	--flash_size 16MB \
    0x1000 "$(BUILD_DIR)/sfeDataLoggerIoT.ino.bootloader.bin" \
    0x8000 "$(BUILD_DIR)/sfeDataLoggerIoT.ino.partitions.bin" \
    0x20000 "$(BUILD_DIR)/sfeDataLoggerIoT.ino.bin"
#---------------------------------------------------------------------------------
# Targets
#---------------------------------------------------------------------------------

# Default target - runs a standard build
all: build

# Force a clean rebuild from scratch
build-clean:
	$(MAKE) build ARDUINO_CLEAN=--clean

# Main build target:
#   - Resolves FLUX_SDK_PATH from ../flux-sdk if not already set
#   - Creates the build directory if it doesn't exist
#   - Runs cmake, then compiles with arduino-cli
build:
	@if [ -z "$$FLUX_SDK_PATH" ] && [ -d "../flux-sdk" ]; then \
		export FLUX_SDK_PATH=$$(realpath ../flux-sdk); \
		echo "FLUX_SDK_PATH set to $$FLUX_SDK_PATH"; \
	fi; \
	if [ -z "$$FLUX_SDK_PATH" ]; then \
		echo "Error: FLUX_SDK_PATH is not set and ../flux-sdk does not exist."; \
		exit 1; \
	fi; \
	if [ ! -d "build" ]; then \
		echo "Creating build directory"; \
		mkdir build; \
	fi; \
	echo "Running cmake"; \
	cd build && cmake .. && cd ..; \
	echo ""; \
	echo "Building..."; \
	echo "$(ARDUINO_CMD)"; \
	$(ARDUINO_CMD)

# Install all Arduino library dependencies via arduino-cli.
# Usage: make depends
depends:
	@echo "Installing Arduino library dependencies..."; \
	for lib in $(ARDUINO_LIBS); do \
		echo "Installing $$lib..."; \
		arduino-cli lib install $$lib; \
	done; \
	echo "Done installing dependencies."

# Flash the firmware to the device via esptool.py.
# Usage: make upload PORT=/dev/tty.usbserial-5110
upload:
	@if [ -z "$(PORT)" ]; then \
		echo "Error: PORT is required. Usage: make upload PORT=/dev/tty.usbserial-5110"; \
		exit 1; \
	fi; \
	echo "Uploading to $(PORT) at $(UPLOAD_BAUD) baud..."; \
	echo "$(UPLOAD_CMD)"; \
	$(UPLOAD_CMD)

# Remove all build artifacts listed in CLEAN_DIRS, if they exist
clean:
	@for dir in $(CLEAN_DIRS); do \
		if [ -d "$$dir" ]; then \
			echo "Removing $$dir"; \
			rm -rf "$$dir"; \
		fi; \
	done