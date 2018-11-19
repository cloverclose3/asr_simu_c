IDIR =include
CC=arm-linux-gcc
#CC=gcc
CFLAGS= -std=c99 -I$(IDIR)

TARGET = asr_simu

ODIR=obj
#LDIR =../lib
LDIR =

LIBS=-lcurl 

_DEPS = 
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = \
asr_simu.o \
base64_util.o \
cJSON.o

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS) prebuild
	$(CC) -c -o $@ $< $(CFLAGS)

asr_simu: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean prebuild

clean:
	rm -f $(TARGET) $(ODIR)/*.o *~ core $(INCDIR)/*~ 
prebuild:
	mkdir -p include obj
