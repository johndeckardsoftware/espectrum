PROJECT_DIR := $(abspath ..)
MAIN_DIR := $(PROJECT_DIR)/main
BUILD_DIR := $(abspath .)
OBJ_DIR := $(BUILD_DIR)/obj
TARGET := $(BUILD_DIR)/espectrum-ubuntu

CXX := g++
CXXFLAGS := -std=c++20 -O0 -g -Wall -Wextra \
	-DESPECTRUM_DESKTOP \
	-I$(MAIN_DIR) \
	-I$(BUILD_DIR)/raylib/include
LDFLAGS :=
LDLIBS := -L$(BUILD_DIR)/raylib -lraylib -lz \
	-lGL -lm -lpthread -ldl -lrt \
	-lX11 -lXrandr -lXinerama -lXi -lXcursor

SOURCES := \
	$(MAIN_DIR)/ubuntu/ubuntu.cpp \
	$(MAIN_DIR)/ubuntu/audio.cpp \
	$(MAIN_DIR)/ubuntu/display.cpp \
	$(MAIN_DIR)/ubuntu/config.cpp \
	$(MAIN_DIR)/common/helper.cpp \
	$(MAIN_DIR)/spectrum/48k_rom.cpp \
	$(MAIN_DIR)/spectrum/AySound.cpp \
	$(MAIN_DIR)/spectrum/keyboard.cpp \
	$(MAIN_DIR)/spectrum/snaps.cpp \
	$(MAIN_DIR)/spectrum/tape.cpp \
	$(MAIN_DIR)/spectrum/tape_manager.cpp \
	$(MAIN_DIR)/spectrum/tap_tape.cpp \
	$(MAIN_DIR)/spectrum/tzx_tape.cpp \
	$(MAIN_DIR)/spectrum/zx48k.cpp

OBJECTS := $(patsubst $(PROJECT_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
DEPFILES := $(OBJECTS:.o=.d)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(PROJECT_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	$(TARGET) $(ARGS)

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)

-include $(DEPFILES)
