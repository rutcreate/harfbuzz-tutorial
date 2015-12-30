PKGS = harfbuzz cairo-ft freetype2

CFLAGS = `pkg-config --cflags $(PKGS)`
LDFLAGS = `pkg-config --libs $(PKGS)` -lm

all: text2image

%: %.c
	$(CC) -std=c99 -o $@ $^ $(CFLAGS) $(LDFLAGS)
