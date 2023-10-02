PROGRAM := workload
SRC_DIR := source
OBJ_DIR := obj

SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CPPFLAGS := -Iinclude -MMD -MP
LDLIBS := -lpthread

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM) : $(OBJ)
	g++ $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	g++ $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@
	
clean:
	@$(RM) -rv $(OBJ_DIR) $(PROGRAM)
	
-include $(OBJ:.o=.d)
