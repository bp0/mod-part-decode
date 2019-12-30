CFLAGS = -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -Werror=missing-prototypes -Wno-address -Wno-missing-field-initializers

objects = mod_part_decode.o

mpd_test: mpd_test.c $(objects)
		-rm mpd_test
		cc -o mpd_test mpd_test.c $(objects)

mod_part_decode.o: mod_part_decode.h

.PHONY : clean
clean :
	-rm mpd_test $(objects)
