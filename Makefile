OBJECTS = common.o gen_item.o
CFLAGS := -O0 -g -MD -MP -fPIC -m32 -fno-strict-aliasing -Wall -Wno-format-truncation $(CFLAGS)

$(shell mkdir -p build &>/dev/null)

all: build/gen_item.so

clean:
	rm -f $(OBJECTS:%.o=build/%.d)

build/gen_item.so: $(OBJECTS:%.o=build/%.o) build/gen_item.o
	gcc-5 -shared $(CFLAGS) -o $@ $^

build/%.o: %.c
	gcc-5 $(CFLAGS) -c -o $@ $<

-include $(OBJECTS:%.o=build/%.d)
