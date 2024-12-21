TARGET = day_1
SRC = day_1.cu
NVCC_FLAGS = -g -G

$(TARGET): $(SRC)
	nvcc $(NVCC_FLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean
