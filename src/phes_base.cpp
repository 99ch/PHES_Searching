#include "phes_base.h"
#include "coordinates.h"
#include "model2D.h"
#include "reservoir.h"
#include "search_config.hpp"
#include <shapefil.h>
#include <string>

// Function to convert a double to an int with rounding
int convert_to_int(double f)
{
	if(f>=0)
		return (int) (f+0.5);
	else
		return (int) (f-0.5);
}

// Function to find the maximum value in a vector of doubles
double max(vector<double> a)
{
	double amax = -1.0e20;
	for (uint ih=0; ih<a.size(); ih++)
		amax = MAX(amax, a[ih]);

	return amax;
}

// Function to convert dam height and length to volume
double convert_to_dam_volume(int height, double length)
{
	return (((height+freeboard)*(cwidth+dambatter*(height+freeboard)))/1000000)*length;
}

// Overloaded function to convert dam height and length to volume
double convert_to_dam_volume(double height, double length)
{
	return (((height+freeboard)*(cwidth+dambatter*(height+freeboard)))/1000000)*length;
}

// Function to perform linear interpolation
double linear_interpolate(double value, vector<double> x_values, vector<double> y_values)
{
	uint i = 0;
	while (x_values[i]<value-EPS) {
		if (i==x_values.size()-1)
			return INF;
		else
			i++;
	}

	double xlower = (i) ? x_values[i-1] : 0;
	double ylower = (i) ? y_values[i-1] : 0;
	double r = x_values[i]-xlower;

	return (ylower+(y_values[i]-ylower)*(value-xlower)/r);
}

// Function to convert an integer to a string
string str(int i)
{
	char buf[32];
	sprintf(buf, "%d", i);
	string to_return(buf);
	return to_return;
}

// Function to get the current wall time in microseconds
unsigned long walltime_usec()
{
	struct timeval now;
	gettimeofday(&now,(struct timezone*)0);
	return (1000000*now.tv_sec + now.tv_usec);
}

// Function to find the required volume based on energy and head
double find_required_volume(int energy, int head)
{
	return (((double)(energy)*J_GWh_conversion)/((double)(head)*water_density*gravity*generation_efficiency*usable_volume*cubic_metres_GL_conversion));
}

// Function to convert a string to a char array (with memory leak)
char* convert_string(const string& str){
  // NUKE THIS, mem leak galore
	char *c = new char[str.length() + 1];
	strcpy(c, str.c_str());
	return c;
}

// Function to convert a double to a string with a specified number of decimal places
string dtos(double f, int nd) {
	stringstream ss;
	ss << fixed << std::setprecision(nd) << f;
	return ss.str();
}

// Function to read DEM data with borders
Model<short>* read_DEM_with_borders(GridSquare sc, int border){
	Model<short>* DEM = new Model<short>(0, 0, MODEL_UNSET);
	const int neighbors[9][4][2] = {
		//[(Tile coordinates) , (Tile base)		 		  , (Tile limit)				  , (Tile offset)	 	       ]
		{ {sc.lat  ,sc.lon  } , {border,      border	 }, {border+3600,  	3600+border	 }, {border-1,    border     } },
		{ {sc.lat+1,sc.lon-1} , {0,			  0		 	 }, {border, 	    border	 	 }, {border-3601, border-3600} },
		{ {sc.lat+1,sc.lon  } , {0,	      	  border	 }, {border,	    3600+border	 }, {border-3601, border     } },
		{ {sc.lat+1,sc.lon+1} , {0,	      	  3600+border}, {border,        3600+2*border}, {border-3601, border+3600} },
		{ {sc.lat  ,sc.lon+1} , {border-1,    3600+border}, {3600+border,   3600+2*border}, {border-1,    border+3600} },
		{ {sc.lat-1,sc.lon+1} , {3600+border, 3600+border}, {3600+2*border, 3600+2*border}, {border+3599, border+3600} },
		{ {sc.lat-1,sc.lon  } , {3600+border, border	 }, {3600+2*border, 3601+border	 }, {border+3599, border     } },
		{ {sc.lat-1,sc.lon-1} , {3600+border, 0		 	 }, {3600+2*border, border	 	 }, {border+3599, border-3600} },
		{ {sc.lat  ,sc.lon-1} , {border-1,    0		 	 }, {3600+border,   border	 	 }, {border-1,    border-3600} }
	};
	for (int i=0; i<9; i++) {
		GridSquare gs = GridSquare_init(neighbors[i][0][0], neighbors[i][0][1]);
		ArrayCoordinate tile_start = ArrayCoordinate_init(neighbors[i][1][0], neighbors[i][1][1], get_origin(gs, border));
		ArrayCoordinate tile_end = ArrayCoordinate_init(neighbors[i][2][0], neighbors[i][2][1], get_origin(gs, border));
		ArrayCoordinate tile_offset = ArrayCoordinate_init(neighbors[i][3][0], neighbors[i][3][1], get_origin(gs, border));
		try{
			Model<short>* DEM_temp = new Model<short>(file_storage_location+"input/DEMs/"+str(gs)+"_1arc_v3.tif", GDT_Int16);
			if (i==0) {
				DEM = new Model<short>(DEM_temp->nrows()+2*border-1,DEM_temp->ncols()+2*border-1, MODEL_SET_ZERO);
				DEM->set_geodata(DEM_temp->get_geodata());
				GeographicCoordinate origin = get_origin(gs, border);
				DEM->set_origin(origin.lat, origin.lon);
			}
			for(int row = tile_start.row ; row < tile_end.row ; row++)
				for(int col = tile_start.col ; col < tile_end.col; col++)
					DEM->set(row, col, DEM_temp->get(row-tile_offset.row,col-tile_offset.col));
			delete DEM_temp;
		}catch (int e){
			search_config.logger.debug("Could not find file "+file_storage_location+"input/DEMs/"+str(gs)+"_1arc_v3.tif " + strerror(errno));
			if (i==0)
				throw(1);
		}
	}
	return DEM;
}

// Function to initialize a BigModel structure

BigModel BigModel_init(GridSquare sc){
	BigModel big_model;
	GridSquare neighbors[9] = {
		(GridSquare){sc.lat  ,sc.lon  },
		(GridSquare){sc.lat+1,sc.lon-1},
		(GridSquare){sc.lat+1,sc.lon  },
		(GridSquare){sc.lat+1,sc.lon+1},
		(GridSquare){sc.lat  ,sc.lon+1},
		(GridSquare){sc.lat-1,sc.lon+1},
		(GridSquare){sc.lat-1,sc.lon  },
		(GridSquare){sc.lat-1,sc.lon-1},
		(GridSquare){sc.lat  ,sc.lon-1}};
	for(int i = 0; i<9; i++){
		big_model.neighbors[i] = neighbors[i];
	}
	big_model.DEM = read_DEM_with_borders(sc, 3600);
	for(int i = 0; i<9; i++){
		GridSquare gs = big_model.neighbors[i];
		try{
			big_model.flow_directions[i] = new Model<char>(file_storage_location+"processing_files/flow_directions/"+str(gs)+"_flow_directions.tif",GDT_Byte);
		}catch(int e){
			search_config.logger.debug("Could not find " + str(gs));
		}
	}
	return big_model;
}

// Function to calculate the cost of a power house
double calculate_power_house_cost(double power, double head){
	return powerhouse_coeff*pow(MIN(power,800),(power_exp))/pow(head,head_exp);
}

// Function to calculate the cost of a tunnel
double calculate_tunnel_cost(double power, double head, double seperation){
	return ((power_slope_factor*MIN(power,800)+slope_int)*pow(head,head_coeff)*seperation*1000)+(power_offset*MIN(power,800)+tunnel_fixed);
}

// Function to set the Figure of Merit (FOM) for a pair of reservoirs
void set_FOM(Pair* pair){
	double seperation = pair->distance;
	double head = (double)pair->head;
	double power = 1000*pair->energy_capacity/pair->storage_time;
	double energy_cost = dam_cost*1/(pair->water_rock*generation_efficiency * usable_volume*water_density*gravity*head)*J_GWh_conversion/cubic_metres_GL_conversion;
	double power_cost;
	double tunnel_cost;
	double power_house_cost;
	if (head > 800) {
		power_house_cost = 2*calculate_power_house_cost(power/2, head/2);
		tunnel_cost = 2*calculate_tunnel_cost(power/2, head/2, seperation);
		power_cost = 0.001*(power_house_cost+tunnel_cost)/MIN(power, 800);
	}
	else {
		power_house_cost = calculate_power_house_cost(power, head);
		tunnel_cost = calculate_tunnel_cost(power, head, seperation);
		power_cost = 0.001*(power_house_cost+tunnel_cost)/MIN(power, 800);
		if(pair->lower.ocean){
			double total_lining_cost = lining_cost*pair->upper.area*meters_per_hectare;
			power_house_cost = power_house_cost*sea_power_scaling;
			double marine_outlet_cost = ref_marine_cost*power*ref_head/(ref_power*head);
			power_cost = 0.001*((power_house_cost+tunnel_cost)/MIN(power, 800) + marine_outlet_cost/power);
			energy_cost += 0.000001*total_lining_cost/pair->energy_capacity;
		}
	}

	pair->FOM = power_cost+energy_cost*pair->storage_time;
	pair->category = 'Z';
	uint i = 0;
	while(i<category_cutoffs.size() && pair->FOM<category_cutoffs[i].power_cost+pair->storage_time*category_cutoffs[i].storage_cost){
		pair->category = category_cutoffs[i].category;
		i++;
	}
}

// Function to convert energy capacity to a string
string energy_capacity_to_string(double energy_capacity){
	if(energy_capacity<10-EPS)
		return dtos(energy_capacity,1);
	else
		return to_string(convert_to_int(energy_capacity));
}

// Function to convert a Test structure to a string
string str(Test test){
	return energy_capacity_to_string(test.energy_capacity)+"GWh_"+to_string(test.storage_time)+"h";
}

// Function to check if a file exists (using char* as input)
bool file_exists (char* name) {
	ifstream infile(name);
    return infile.good();
}

// Function to check if a file exists (using string as input)
bool file_exists (string name) {
	ifstream infile(name.c_str());
    return infile.good();
}

// Function to get the origin of a geographic coordinate with a border

GeographicCoordinate get_origin(double latitude, double longitude, int border){
	return GeographicCoordinate_init(FLOOR(latitude)+1+(border/3600.0),FLOOR(longitude)-(border/3600.0));
}

ExistingReservoir get_existing_reservoir(string name, string filename) {
  ExistingReservoir to_return;
  int i = 0;
  // If filename is empty, set it to the default path
  if (filename.empty())
    filename = file_storage_location + "input/existing_reservoirs/" + existing_reservoirs_csv;
  // Check if the file exists
  if(!file_exists(convert_string(filename))){
    cout << "File " << filename << " does not exist." << endl;
    throw 1;
  }

  // If using tiled bluefield data
  if (use_tiled_bluefield) {
    DBFHandle DBF = DBFOpen(convert_string(filename), "rb");
    int dbf_field = DBFGetFieldIndex(DBF, string("Vol_total").c_str());
    int dbf_elevation_field = DBFGetFieldIndex(DBF, string("Elevation").c_str());
    int dbf_name_field = DBFGetFieldIndex(DBF, string("Lake_name").c_str());
    // Loop through DBF records to find the reservoir by name
    for (i = 0; i<DBFGetRecordCount(DBF); i++){
      const char* s = DBFReadStringAttribute(DBF, i, dbf_name_field);
      if(name==string(s)){
        break;
      }
    }
    // If the reservoir is not found, throw an error
    if(i==DBFGetRecordCount(DBF)){
      cout<<"Could not find reservoir with name " << name << " in " << filename << endl;
      throw 1;
    }
    // Initialize the reservoir with the found attributes
    to_return = ExistingReservoir_init(name, 0, 0, DBFReadIntegerAttribute(DBF, i, dbf_elevation_field), DBFReadDoubleAttribute(DBF, i, dbf_field));
    DBFClose(DBF);
  } else {
    // Read existing reservoir data from the file
    vector<ExistingReservoir> reservoirs = read_existing_reservoir_data(convert_string(filename));

    bool found = false;
    // Loop through the reservoirs to find the one with the matching name
    for (ExistingReservoir r : reservoirs)
      if (r.identifier == name){
        found = true;
        to_return = r;
        break;
      }
    // If the reservoir is not found, throw an error
    if(!found){
      cout<<"Could not find reservoir with name " << name << " in " << filename << endl;
      throw 1;
    }

    // Loop through the names to find the index of the matching name
    for (string s : read_names(convert_string(file_storage_location + "input/existing_reservoirs/" +
                                              existing_reservoirs_shp_names))) {
      if (s == name)
        break;
      else
        i++;
    }
  }

  // Check if the shapefile exists
  if (!file_exists(filename)) {
    search_config.logger.debug("No file: " + filename);
    throw(1);
  }
  SHPHandle SHP = SHPOpen(convert_string(filename), "rb");
  if (SHP != NULL) {
    int nEntities;
    SHPGetInfo(SHP, &nEntities, NULL, NULL, NULL);

    SHPObject *shape;
    shape = SHPReadObject(SHP, i);
    if (shape == NULL) {
      fprintf(stderr, "Unable to read shape %d, terminating object reading.\n",
              i);
      throw(1);
    }
    // Loop through the vertices of the shape to create the polygon
    for (int j = 0; j < shape->nVertices; j++) {
      // if(shape->panPartStart[iPart] == j )
      //  break;
      GeographicCoordinate temp_point =
          GeographicCoordinate_init(shape->padfY[j], shape->padfX[j]);
      to_return.polygon.push_back(temp_point);
    }
    SHPDestroyObject(shape);
  } else {
    cout << "Could not read shapefile " << filename << endl;
    throw(1);
  }
  SHPClose(SHP);

  // Calculate the area of the polygon
  to_return.area = geographic_polygon_area(to_return.polygon);

  return to_return;
}

ExistingReservoir get_existing_tiled_reservoir(string name, double lat, double lon) {
  // Initialize the grid square based on latitude and longitude
  GridSquare grid_square = GridSquare_init(convert_to_int(lat-0.5), convert_to_int(lon-0.5));
  // Construct the filename for the shapefile tile
  string filename = file_storage_location + "input/bluefield_shapefile_tiles/" +
                      str(grid_square) + "_shapefile_tile.shp";
  // Get the existing reservoir from the tile
  return get_existing_reservoir(name, filename);
}

vector<ExistingReservoir> get_existing_reservoirs(GridSquare grid_square) {
  vector<string> filenames;
  vector<ExistingReservoir> to_return;
  vector<string> names;
  vector<ExistingReservoir> reservoirs;

  // Add filenames based on the configuration
  if (use_tiled_rivers) {
    filenames.push_back(file_storage_location + "input/river_shapefile_tiles/" + str(grid_square) +
                        "_shapefile_tile.shp");
  }
  if (use_tiled_bluefield) {
    filenames.push_back(file_storage_location + "input/bluefield_shapefile_tiles/" +
                        str(grid_square) + "_shapefile_tile.shp");
  }
  if (!use_tiled_bluefield && !use_tiled_rivers) {
    string csv_filename =
        file_storage_location + "input/existing_reservoirs/" + existing_reservoirs_csv;
    if (!file_exists(csv_filename)) {
      cout << "File " << csv_filename << " does not exist." << endl;
      throw 1;
    }
    reservoirs = read_existing_reservoir_data(csv_filename.c_str());

    names = read_names(convert_string(file_storage_location + "input/existing_reservoirs/" +
                                      existing_reservoirs_shp_names));

    filenames.push_back(file_storage_location + "input/existing_reservoirs/" +
                        existing_reservoirs_shp);
  }

  // Loop through the filenames to read the shapefiles and DBF files
  for (string filename : filenames) {
    if (!file_exists(filename)) {
      search_config.logger.error("No file: " + filename);
      throw(1);
    }
    SHPHandle SHP = SHPOpen(convert_string(filename), "rb");
    DBFHandle DBF = DBFOpen(convert_string(filename), "rb");
    bool tiled_bluefield = use_tiled_bluefield && SHP->nShapeType == SHPT_POLYGON;
    bool tiled_river = use_tiled_rivers && SHP->nShapeType == SHPT_ARC;
    bool csv_names = !tiled_bluefield && !tiled_river;
    int dbf_field = 0, dbf_name_field = 0, dbf_elevation_field = 0;
    if (tiled_river) {
      dbf_field = DBFGetFieldIndex(DBF, string("DIS_AV_CMS").c_str());
      dbf_name_field = DBFGetFieldIndex(DBF, string("River_name").c_str());
    }
    if (tiled_bluefield) {
      dbf_field = DBFGetFieldIndex(DBF, string("Vol_total").c_str());
      dbf_elevation_field = DBFGetFieldIndex(DBF, string("Elevation").c_str());
      dbf_name_field = DBFGetFieldIndex(DBF, string("Lake_name").c_str());
    }
    if (SHP != NULL) {
      int nEntities;
      SHPGetInfo(SHP, &nEntities, NULL, NULL, NULL);

      // Loop through the entities in the shapefile
      for (int i = 0; i < nEntities; i++) {
        SHPObject *shape;
        shape = SHPReadObject(SHP, i);
        if (shape == NULL) {
          fprintf(stderr, "Unable to read shape %d, terminating object reading.\n", i);
        } else {
          ExistingReservoir reservoir;
          if (csv_names) {
            int idx = -1;
            // Find the reservoir by name
            for (uint r = 0; r < reservoirs.size(); r++) {
              if (reservoirs[r].identifier == names[i]) {
                idx = r;
              }
            }
            if (idx < 0) {
              search_config.logger.debug("Could not find reservoir with id " + names[i]);
              throw 1;
            }
            reservoir = reservoirs[idx];
          } else if (tiled_bluefield) {
            double volume = DBFReadDoubleAttribute(DBF, i, dbf_field);
            int elevation = DBFReadIntegerAttribute(DBF, i, dbf_elevation_field);
            string name = string(DBFReadStringAttribute(DBF, i, dbf_name_field));
            reservoir = ExistingReservoir_init(name, 0, 0, elevation, volume);
          } else if (tiled_river) {
            double volume = DBFReadDoubleAttribute(DBF, i, dbf_field);
            string name = string(DBFReadStringAttribute(DBF, i, dbf_name_field));
            reservoir = ExistingReservoir_init(name, 0, 0, 0, volume);
            reservoir.river = true;
          }
          // Loop through the vertices of the shape to create the polygon
          for (int j = 0; j < shape->nVertices; j++) {
            // if(shape->panPartStart[iPart] == j )
            //  break;
            GeographicCoordinate temp_point =
                GeographicCoordinate_init(shape->padfY[j], shape->padfX[j]);
            reservoir.polygon.push_back(temp_point);
          }
          // Coordinates in existing_reservoirs_csv are based on geometric centre.
          // Require the same calculation of coordinates here to prevent
          // disconnect between reservoirs.csv and existing_reservoirs_csv
          bool overlaps_grid_cell = false;
          double centre_gc_lat = 0;
          double centre_gc_lon = 0;

          for (GeographicCoordinate gc : reservoir.polygon) {
            centre_gc_lat += gc.lat;
            centre_gc_lon += gc.lon;
          }
          GeographicCoordinate centre_gc = GeographicCoordinate_init(
              centre_gc_lat / reservoir.polygon.size(), centre_gc_lon / reservoir.polygon.size());
          if (check_within(centre_gc, grid_square)) {
            overlaps_grid_cell = true;
          }
          if (!csv_names) {
            reservoir.latitude = centre_gc.lat;
            reservoir.longitude = centre_gc.lon;
          }

          SHPDestroyObject(shape);
          if (overlaps_grid_cell) {
            reservoir.area = geographic_polygon_area(reservoir.polygon);
            to_return.push_back(reservoir);
          }
        }
      }
    } else {
      cout << "Could not read shapefile " << filename << endl;
      throw(1);
    }
    SHPClose(SHP);
    DBFClose(DBF);
  }
  return to_return;
}

RoughBfieldReservoir existing_reservoir_to_rough_reservoir(ExistingReservoir r) {
  RoughBfieldReservoir reservoir;
  reservoir.identifier = r.identifier;
  reservoir.brownfield = true;
  reservoir.river = r.river;
  reservoir.ocean = false;
  reservoir.latitude = r.latitude;
  reservoir.longitude = r.longitude;
  reservoir.elevation = r.elevation;
  reservoir.bottom_elevation = r.elevation;
  // Loop through dam wall heights and initialize reservoir properties
  for (uint i = 0; i < dam_wall_heights.size(); i++) {
    reservoir.volumes.push_back(r.volume);
    reservoir.dam_volumes.push_back(0);
    reservoir.areas.push_back(r.area);
    reservoir.water_rocks.push_back(1000000000);
  }

	GeographicCoordinate origin = get_origin(r.latitude, r.longitude, border);
	for(GeographicCoordinate c : r.polygon)
    reservoir.shape_bound.push_back(convert_coordinates(c, origin));
	return reservoir;
}

vector<ExistingPit> get_pit_details(GridSquare grid_square){
	vector<ExistingPit> gridsquare_pits;

	vector<ExistingPit> pits = read_existing_pit_data(convert_string(file_storage_location+"input/existing_reservoirs/"+existing_reservoirs_csv));

	for(ExistingPit p : pits){
		if (check_within(GeographicCoordinate_init(p.reservoir.latitude, p.reservoir.longitude), grid_square))
			gridsquare_pits.push_back(p);
	}
	return gridsquare_pits;
}

ExistingPit get_pit_details(string pitname){
	ExistingPit pit;
	vector<ExistingPit> pits = read_existing_pit_data(convert_string(file_storage_location+"input/existing_reservoirs/"+existing_reservoirs_csv));

	for(ExistingPit p : pits){
		if (p.reservoir.identifier==pitname)
			pit = p;
	}
	return pit;
}

void depression_volume_finding(Model<short>* DEM) {
	vector<vector<string> > csv_modified_lines;
	vector<int> csv_modified_line_numbers;
	string filename = file_storage_location + "input/existing_reservoirs/" + existing_reservoirs_csv;

	if (!file_exists(convert_string(filename))) {
		cout << "File " << filename << " does not exist." << endl;
		throw 1;
	}
	vector<ExistingReservoir> reservoirs = read_existing_reservoir_data(convert_string(filename));

	vector<string> names = read_names(convert_string(
		file_storage_location + "input/existing_reservoirs/" + existing_reservoirs_shp_names));

	filename = file_storage_location + "input/existing_reservoirs/" + existing_reservoirs_shp;

	char *shp_filename = new char[filename.length() + 1];
	strcpy(shp_filename, filename.c_str());
	if (!file_exists(shp_filename)) {
		search_config.logger.debug("No file: " + filename);
		throw(1);
	}

	SHPHandle SHP = SHPOpen(convert_string(filename), "rb");
	if (SHP != NULL) {
		int nEntities;
		SHPGetInfo(SHP, &nEntities, NULL, NULL, NULL);

		SHPObject *shape;

		for(int i = 0; i<nEntities; i++){
			Model<bool>* extent = new Model<bool>(DEM->nrows(), DEM->ncols(), MODEL_SET_ZERO);
			extent->set_geodata(DEM->get_geodata());
			short min_elevation = 32767;
			short max_elevation = 0;
			vector<string> csv_modified_line(2*num_altitude_volume_pairs+2);

			shape = SHPReadObject(SHP, i);
			if (shape == NULL) {
				fprintf(stderr, "Unable to read shape %d, terminating object reading.\n", i);
			}
			int idx = -1;
			for (uint r = 0; r < reservoirs.size(); r++) {
				if (reservoirs[r].identifier == names[i]) {
					idx = r;
				}
			}
			if (idx < 0) {
				search_config.logger.debug("Could not find reservoir with id " + names[i]);
        exit(1);
			}

			ExistingReservoir reservoir = reservoirs[idx];
			GeographicCoordinate gc = GeographicCoordinate_init(reservoir.latitude, reservoir.longitude);
			if(!check_within(gc, search_config.grid_square)) {
				SHPDestroyObject(shape);
				delete extent;
				continue;
			}
			vector<GeographicCoordinate> temp_poly;
			for (int j = 0; j < shape->nVertices; j++) {
				GeographicCoordinate temp_point = GeographicCoordinate_init(shape->padfY[j], shape->padfX[j]);
				temp_poly.push_back(temp_point);

			}
			polygon_to_raster(temp_poly, extent);
			SHPDestroyObject(shape);

			// Find lowest elevation within mine polygon (pour point)
			for(int row = 0; row<extent->nrows(); row++)
				for(int col = 0; col<extent->ncols(); col++){
					if(extent->get(row, col)) {
						min_elevation = MIN(DEM->get(row, col), min_elevation);
						max_elevation = MAX(DEM->get(row,col), max_elevation);
					}
				}

			double area_at_elevation[max_elevation + 1];
			double volume_at_elevation[max_elevation + 1];
			double cumulative_area_at_elevation[max_elevation + 1];
			double pit_elevations[num_altitude_volume_pairs];
      std::memset(area_at_elevation, 0, (max_elevation+1)*sizeof(double));
      std::memset(volume_at_elevation, 0, (max_elevation+1)*sizeof(double));
      std::memset(cumulative_area_at_elevation, 0, (max_elevation+1)*sizeof(double));
      std::memset(pit_elevations, 0, (num_altitude_volume_pairs)*sizeof(double));

			// Determine the elevations for altitude-volume pairs
			for (int ih = 1; ih <= num_altitude_volume_pairs; ih++) {
				pit_elevations[ih-1] = min_elevation + std::round(ih * (max_elevation - min_elevation)/num_altitude_volume_pairs);
			}

			// Find the area of cells within mine polygon at each elevation above the pour point
			for(int row = 0; row<extent->nrows(); row++)
				for(int col = 0; col<extent->ncols(); col++)
					if(extent->get(row, col)){
						area_at_elevation[min_elevation + 1] += find_area(ArrayCoordinate_init(row, col, DEM->get_origin()));
					}

			// Find the surface area and volume of reservoir at each elevation above pour point
			for (int ih=1; ih<max_elevation+1-min_elevation;ih++) {
				cumulative_area_at_elevation[min_elevation + ih] = cumulative_area_at_elevation[min_elevation + ih-1] + area_at_elevation[min_elevation + ih];
				volume_at_elevation[min_elevation + ih] = volume_at_elevation[min_elevation + ih-1] + 0.01*cumulative_area_at_elevation[min_elevation + ih]; // area in ha, vol in GL
			}

			// Find the altitude-volume pairs for the pit
			csv_modified_line[0] = to_string(min_elevation);
			csv_modified_line[1] = to_string(volume_at_elevation[max_elevation]);
			for (int ih =0 ; ih < num_altitude_volume_pairs; ih++) {
				int height = pit_elevations[ih];
				csv_modified_line[2+2*ih] = to_string(height);
				csv_modified_line[2+2*ih + 1] = to_string(volume_at_elevation[height]);
			}

			// Add the line to the vector to be written to the pits CSV
			csv_modified_lines.push_back(csv_modified_line);
			csv_modified_line_numbers.push_back(i+1);

			//extent->write(file_storage_location+"debug/extent/"+str(search_config.grid_square)+"_extent.tif", GDT_Byte);

			delete extent;
			temp_poly.clear();
		}
	} else {
		cout << "Could not read shapefile " << filename << endl;
		throw(1);
	}
	SHPClose(SHP);

	// Write the altitude-volume pairs to the CSV
	std::ifstream inputFile(file_storage_location + "input/existing_reservoirs/" + existing_reservoirs_csv);
	vector<string> lines;

	if (!inputFile.is_open()) {
		printf("Error opening pits CSV\n");
	}

	string line;
	int line_number = 0;
	while(std::getline(inputFile, line)) {
		istringstream lineStream(line);
		string cell;
		int column = 1;
		ostringstream modifiedLine;

		while (std::getline(lineStream, cell, ',')) {

			if (column >= 4 && column <= 5 + 2*num_altitude_volume_pairs && std::count(csv_modified_line_numbers.begin(), csv_modified_line_numbers.end(), line_number)) {
				std::vector<int>::iterator vector_index_itr = find(csv_modified_line_numbers.begin(), csv_modified_line_numbers.end(), line_number);
				int vector_index = std::distance(csv_modified_line_numbers.begin(), vector_index_itr);
				cell = string(csv_modified_lines[vector_index][column-4]);
			}

			modifiedLine << cell;

			if (column < 5 + 2*num_altitude_volume_pairs) {
				modifiedLine << ",";
			}

			++column;
		}

		lines.push_back(modifiedLine.str());

		line_number++;
	}

	inputFile.close();

	std::ofstream outputFile(file_storage_location + "input/existing_reservoirs/" + existing_reservoirs_csv);
	for (const auto &line : lines) {
		outputFile << line << std::endl;
	}

	outputFile.close();
}

