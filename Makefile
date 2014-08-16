PROGRAM = gl_noise
OBJS = main.o \
	camera.o noise_gen.o noise_renderer.o shader_utils.o vector_math.o
HEADERS = camera.h noise_gen.h noise_renderer.h shader_utils.h vector_math.h

CFLAGS += -std=c99 -Wall -Wextra -pedantic
LDLIBS += -lm -lGLEW -lGL -lglfw

.PHONY: all clean

all: $(PROGRAM)

clean:
	rm -f $(OBJS) $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(LINK.o) $^ $(LDLIBS) -o $@

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@
