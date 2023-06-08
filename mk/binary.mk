chpst_OBJECTS := parse.o util.o

finit_OBJECTS := message.o util.o

fsvc_OBJECTS := message.o util.o decode.o signame.o

fsvs_OBJECTS := message.o util.o supervise.o service.o start.o stop.o \
                register.o handle_exit.o handle_command.o encode.o \
                parse.o dependency.o pattern.o status.o

halt_OBJECTS := wtmp.o util.o

modules-load_OBJECTS := util.o

seedrng_OBJECTS := 

sigremap_OBJECTS := message.o signame.o

vlogger_OBJECTS := message.o

zzz_OBJECTS :=

chpst_FLAGS :=
finit_FLAGS := -static
fsvc_FLAGS :=
fsvs_FLAGS :=
halt_FLAGS :=
modules-load_FLAGS :=
seedrng_FLAGS :=
sigremap_FLAGS :=
vlogger_FLAGS :=
zzz_FLAGS :=

