SRC_DIR := ./src
OBJ_DIR := ../bin/obj
BIN_DIR := ../bin/Debug
DEV_DIR := C:/Devops/cpp/libraries

EXE := $(BIN_DIR)/test.exe
SRC := $(wildcard $(SRC_DIR)/*.cpp)
# OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
OBJ := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.obj, $(SRC))

INCLUDE_PATH := -I../code/include \
	-I$(DEV_DIR)/SDL2/include \
	-I$(DEV_DIR)/VulkanSDK/1.3.216.0/Include \
	-I$(DEV_DIR)/VulkanSDK/1.3.216.0/Include/vma \
	-I$(DEV_DIR)/stb-master
LIB_PATH := -LIBPATH:$(DEV_DIR)/SDL2/lib/x64 \
	-LIBPATH:$(DEV_DIR)/VulkanSDK/1.3.216.0/Lib
LIBRARY := SDL2main.lib SDL2.lib Shell32.lib \
	vulkan-1.lib
# NEVER $(LIB) because LIB is a window environment variable!!!

CXX := cl
CPPFLAGS := $(INCLUDE_PATH) -MP -std:c++17 -nologo -EHsc -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -FC

# Debug
CXXFLAGS := /MDd /ZI /Ob0 /Od /RTC1
LDFLAGS  := -DEBUG:FULL -MAP:$(OBJ_DIR)/test.map -subsystem:console $(LIB_PATH)

# Release
# CXXFLAGS := /MD /Zi /O2 /Ob1 /DNDEBUG
# LDFLAGS  := -opt:ref -subsystem:console $(LIB_PATH)

# CFLAGS   :=
LDLIBS   := $(LIBRARY)

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ) | $(BIN_DIR)
	link $(LDFLAGS) $^ $(LDLIBS) /out:$@

$(OBJ_DIR)/%.obj: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) /c $< /Fo$@

$(BIN_DIR):
	mkdir $(subst /,\,$(BIN_DIR))

$(OBJ_DIR):
	mkdir $(subst /,\,$(OBJ_DIR))

clean:
	@rmdir /q /s $(subst /,\,$(BIN_DIR)) 
	@rmdir /q /s $(subst /,\,$(OBJ_DIR))

-include $(OBJ:.obj=.d)

#BUILDWIN=x64
#32-bit build
#ifeq ($(BUILDWIN), x86)
#ExtLibPathX86=-LIBPATH:"$(DEV_DIR)/SDL2/lib/x86" -LIBPATH:$(DEV_DIR)/SDL2_image/lib/x86 -LIBPATH:$(DEV_DIR)/SDL2_ttf/lib/x86 -LIBPATH:$(DEV_DIR)/SDL2_mixer/lib/x86 -LIBPATH:"$(DEV_DIR)/glew/lib/Release/Win32"

#$(TARGET)/main.exe: $(addprefix $(TARGET)/, $(OBJFILEs))
#	$(Linker) $(LNKFLAGs) -subsystem:windows,5.1 $(ExtLibPathX86) $(ExtLibraries) $(addprefix $(TARGET)/, $(OBJFILEs)) -out:$@
#endif