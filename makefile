# Compiler and flags
COMP = g++
FLAGS = -std=c++11 -g

# Directories and libraries
LIB_DIR = /home/tasos/Documents/visual_fractals/MusicBeatDetector_repo/build/MusicBeatDetector/bin/Release/
LIBS = -L$(LIB_DIR) -lMusicBeatDetector -lportaudio -lfftw3f -lblas -lsndfile -lasound -lmp3lame -ldl -lpthread -lm -lGL -lGLU -lglfw -lGLEW

# Source files and objects
SRC = main.cpp sound_analysis.cpp
OBJ = main.o sound_analysis.o

# Output executable
EXEC = ./fractal

# Default target
all: $(EXEC)

# Compile sound_analysis.cpp to sound_analysis.o
sound_analysis.o: sound_analysis.cpp
	$(COMP) $(FLAGS) -c sound_analysis.cpp -o sound_analysis.o

# Compile main.cpp to main.o
main.o: main.cpp
	$(COMP) $(FLAGS) -c main.cpp -o main.o

# Link the object files to create the executable
$(EXEC): $(OBJ)
	$(COMP) -g $(OBJ) -o $(EXEC) $(LIBS)

# Clean command to remove object files and the executable
clean:
	rm -f $(OBJ) $(EXEC)
