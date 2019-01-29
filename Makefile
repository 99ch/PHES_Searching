
CXXFLAGS = -std=c++11 -O3 -Wall
LIBS = -lgdal -lshp -lm
OBJS1 = src/screening.o src/model2D.o src/TIFF_IO.o src/reservoir.o src/coordinates.o src/phes_base.o 
OBJS2 = src/pairing.o src/model2D.o src/TIFF_IO.o src/reservoir.o src/coordinates.o src/phes_base.o 
OBJS3 = src/pretty_set.o src/reservoir.o src/model2D.o src/TIFF_IO.o src/coordinates.o src/phes_base.o
OBJS4 = src/constructor.o src/reservoir.o src/model2D.o src/TIFF_IO.o src/coordinates.o src/phes_base.o src/kml.o
OBJS5 = src/search_driver.o src/phes_base.o src/coordinates.o src/model2D.o src/TIFF_IO.o
DIRS = bin input output processing_files driver_files
INCDIRS = -Iinclude


utils: $(shell mkdir -p $(DIRS)) bin/screening bin/pairing bin/pretty_set bin/constructor bin/search_driver

bin/screening: $(OBJS1)
	g++ $(CXXFLAGS) $(LDFLAGS) $(OBJS1) $(LIBS) -o $@

bin/pairing: $(OBJS2)
	g++ $(CXXFLAGS) $(LDFLAGS) $(OBJS2) $(LIBS) -o $@

bin/pretty_set: $(OBJS3)
	g++  $(CXXFLAGS) $(LDFLAGS) $(OBJS3) $(LIBS) -o $@

bin/constructor: $(OBJS4)
	g++  $(CXXFLAGS) $(LDFLAGS) $(OBJS4) $(LIBS) -o $@

bin/search_driver: $(OBJS5)
	g++  $(CXXFLAGS) $(LDFLAGS) $(OBJS5) $(LIBS) -o $@

.c.o:
	g++ $(CXXFLAGS) $(INCDIRS) -c -o $@ $<

clean:
	rm -f $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) bin/screening bin/pairing bin/pretty_set bin/constructor bin/search_driver

clear:
	rm -r -f processing_files driver_files && mkdir -p $(DIRS)

run:
	bin/start_drivers.sh 1 tasks processes

run2:
	bin/start_drivers.sh 2 tasks processes

run4:
	bin/start_drivers.sh 4 tasks processes

run8:
	bin/start_drivers.sh 8 tasks processes
