AM_CFLAGS = $(OP_CFLAGS)
EXTRA_DIST= fastintfns.h mjpeg_logging.h mpegconsts.h videodev_mjpeg.h yuv4mpeg_intern.h format_codes.h \
mjpeg_types.h mpegtimecode.h yuv4mpeg.h
MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = -I$(top_srcdir) -I$(includedir) \
              -I$(top_srcdir)/thirdparty/mjpegtools

AM_CPPFLAGS += $(PTHREAD_CFLAGS)

MJPEGUTILS_LIB_FILE = libmjpegutils.la
noinst_LTLIBRARIES = $(MJPEGUTILS_LIB_FILE)
DISTCLEANFILES =
libmjpegutils_la_SOURCES = mjpeg_logging.c mpegconsts.c mpegtimecode.c yuv4mpeg.c yuv4mpeg_ratio.c 
