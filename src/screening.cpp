#include "coordinates.h"
#include "model2D.h"
#include "phes_base.h"
#include "reservoir.h"
#include "search_config.hpp"
#include <climits>

bool debug_output = false;

// Function to read a TIFF filter and update the filter model based on a specific value
void read_tif_filter(string filename, Model<bool>* filter, unsigned char value_to_filter){
	try{
		Model<unsigned char>* tif_filter = new Model<unsigned char>(filename, GDT_Byte);
		GeographicCoordinate point;
		for(int row = 0; row<filter->nrows(); row++){
			for(int col = 0; col<filter->ncols(); col++){
				point = filter->get_coordinate(row, col);
				if(tif_filter->check_within(point) && tif_filter->get(point)==value_to_filter)
					filter->set(row,col,true);
			}
		}
		delete tif_filter;
	}catch(exception& e){
    search_config.logger.debug("Problem with " + filename);
	}catch(int e){
    search_config.logger.debug("Problem with " + filename);
	}
}

// Function to find the UTM filename based on geographic coordinates
string find_world_utm_filename(GeographicCoordinate point){
	char clat = 'A'+floor((point.lat+96)/8);
	if(clat>=73)clat++;
	if(clat>=79)clat++;
	int nlon = floor((point.lon+180)/6+1);
	char strlon[3];
	snprintf(strlon, 3, "%02d", nlon);
	return string(strlon)+string(1, clat);
}

// Function to read filters and update the filter model based on various filenames
Model<bool>* read_filter(Model<short>* DEM, vector<string> filenames)
{
	Model<bool>* filter = new Model<bool>(DEM->nrows(), DEM->ncols(), MODEL_SET_ZERO);
	filter->set_geodata(DEM->get_geodata());
	for(string filename:filenames){
		if(filename=="use_world_urban"){
			search_config.logger.debug("Using world urban data as filter");
			vector<string> done;
			for(GeographicCoordinate corner: DEM->get_corners()){
				string urban_filename = "input/filters/WORLD_URBAN/"+find_world_utm_filename(corner)+"_hbase_human_built_up_and_settlement_extent_geographic_30m.tif";
        if (!file_exists(file_storage_location+urban_filename))
           urban_filename = "input/WORLD_URBAN/"+find_world_utm_filename(corner)+"_hbase_human_built_up_and_settlement_extent_geographic_30m.tif";
				if(find(done.begin(), done.end(), urban_filename)==done.end()){
					read_tif_filter(file_storage_location+urban_filename, filter, 201);
					done.push_back(urban_filename);
				}
			}
		}else if(filename=="use_tiled_filter"){
			search_config.logger.debug("Using tiled filter");
			GridSquare sc = {convert_to_int(FLOOR((filter->get_coordinate(filter->nrows(), filter->ncols()).lat+filter->get_origin().lat)/2.0)),convert_to_int(FLOOR((filter->get_coordinate(filter->nrows(), filter->ncols()).lon+filter->get_origin().lon)/2.0))};
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
        string shp_filename = file_storage_location+"input/shapefile_tiles/"+str(neighbors[i])+"_shapefile_tile.shp";
        if(file_exists(shp_filename))
          read_shp_filter(shp_filename, filter);
        else{
          search_config.logger.debug("Couldn't find file " + shp_filename);
          search_config.logger.debug(to_string(i));
          if(i==0)
            throw(1);

        }
			}
		}else{
			read_shp_filter(file_storage_location+filename, filter);
		}
	}
	for(int row = 0; row<DEM->nrows(); row++)
		for(int col = 0; col<DEM->ncols(); col++)
			if(DEM->get(row,col)<-1000)
				filter->set(row,col,true);
	return filter;
}


Model<double>* fill(Model<short>* DEM)
{
	Model<double>* DEM_filled_no_flat = new Model<double>(DEM->nrows(), DEM->ncols(), MODEL_UNSET);
	DEM_filled_no_flat->set_geodata(DEM->get_geodata());
	Model<bool>* seen = new Model<bool>(DEM->nrows(), DEM->ncols(), MODEL_SET_ZERO);

	queue<ArrayCoordinateWithHeight> q;
	priority_queue<ArrayCoordinateWithHeight> pq;
	for (int row=0; row<DEM->nrows(); row++)
		for (int col=0; col<DEM->ncols();col++)
			DEM_filled_no_flat->set(row,col,(double)DEM->get(row,col));

	for (int row=0; row<DEM->nrows()-1;row++) {
		pq.push(ArrayCoordinateWithHeight_init(row+1, DEM->ncols()-1, (double)DEM->get(row+1,DEM->ncols()-1)));
		seen->set(row+1,DEM->ncols()-1,true);
		pq.push(ArrayCoordinateWithHeight_init(row, 0, (double)DEM->get(row,0)));
		seen->set(row,0,true);
	}

	for (int col=0; col<DEM->ncols()-1;col++) {
		pq.push(ArrayCoordinateWithHeight_init(DEM->nrows()-1, col, (double)DEM->get(DEM->ncols()-1,col)));
		seen->set(DEM->nrows()-1,col,true);
		pq.push(ArrayCoordinateWithHeight_init(0, col+1, (double)DEM->get(0,col+1)));
		seen->set(0,col+1,true);
	}

	ArrayCoordinateWithHeight c;
	ArrayCoordinateWithHeight neighbor;
	while ( !q.empty() || !pq.empty()) {

		if (q.empty()){
			c = pq.top();
			pq.pop();
		}else{
			c = q.front();
			q.pop();
		}

		for (uint d=0; d<directions.size(); d++) {
			neighbor = ArrayCoordinateWithHeight_init(c.row+directions[d].row,c.col+directions[d].col,0);
			if (!DEM->check_within(c.row+directions[d].row, c.col+directions[d].col) || seen->get(neighbor.row,neighbor.col))
				continue;
			neighbor.h = DEM_filled_no_flat->get(neighbor.row,neighbor.col);

			seen->set(neighbor.row,neighbor.col,true);

			if (neighbor.h<=c.h) {
				DEM_filled_no_flat->set(neighbor.row,neighbor.col,DEM_filled_no_flat->get(c.row,c.col) + EPS);
				neighbor.h = DEM_filled_no_flat->get(neighbor.row,neighbor.col);
				q.push(neighbor);
			}
			else {
				pq.push(neighbor);
			}
		}
	}
	delete seen;
	return DEM_filled_no_flat;
}

// Function to find ocean areas in a DEM model
Model<bool>* find_ocean(Model<short>* DEM)
{
	Model<bool>* ocean = new Model<bool>(DEM->nrows(), DEM->ncols(), MODEL_SET_ZERO);
	ocean->set_geodata(DEM->get_geodata());

	Model<bool>* seen = new Model<bool>(DEM->nrows(), DEM->ncols(), MODEL_SET_ZERO);

	queue<ArrayCoordinateWithHeight> q;
	priority_queue<ArrayCoordinateWithHeight> pq;

	for (int row=0; row<DEM->nrows()-1;row++) {
		pq.push(ArrayCoordinateWithHeight_init(row+1, DEM->ncols()-1, (double)DEM->get(row+1,DEM->ncols()-1)));
		seen->set(row+1,DEM->ncols()-1,true);
		if(DEM->get(row+1,DEM->ncols()-1)==0)
			ocean->set(row+1,DEM->ncols()-1,true);
		pq.push(ArrayCoordinateWithHeight_init(row, 0, (double)DEM->get(row,0)));
		seen->set(row,0,true);
		if(DEM->get(row,0)==0)
			ocean->set(row,0,true);
	}

	for (int col=0; col<DEM->ncols()-1;col++) {
		pq.push(ArrayCoordinateWithHeight_init(DEM->nrows()-1, col, (double)DEM->get(DEM->ncols()-1,col)));
		seen->set(DEM->nrows()-1,col,true);
		if(DEM->get(DEM->ncols()-1,col)==0)
			ocean->set(DEM->ncols()-1,col,true);
		pq.push(ArrayCoordinateWithHeight_init(0, col+1, (double)DEM->get(0,col+1)));
		seen->set(0,col+1,true);
		if(DEM->get(0,col+1)==0)
			ocean->set(0,col+1,true);
	}

	ArrayCoordinateWithHeight c;
	ArrayCoordinateWithHeight neighbor;
	while ( !q.empty() || !pq.empty()) {
		if (q.empty()){
			c = pq.top();
			pq.pop();
		}else{
			c = q.front();
			q.pop();
		}

		for (uint d=0; d<directions.size(); d++) {
			neighbor = ArrayCoordinateWithHeight_init(c.row+directions[d].row,c.col+directions[d].col,0);
			if (!DEM->check_within(c.row+directions[d].row, c.col+directions[d].col) || seen->get(neighbor.row,neighbor.col))
				continue;
			neighbor.h = DEM->get(neighbor.row,neighbor.col);

			seen->set(neighbor.row,neighbor.col,true);

			if (neighbor.h<=EPS && neighbor.h>=-EPS && ocean->get(c.row,c.col)==true) {
				ocean->set(neighbor.row,neighbor.col,true);
				q.push(neighbor);
			}
			else {
				pq.push(neighbor);
			}
		}
	}
	delete seen;
	return ocean;
}

// Find the lowest neighbor of a point given the cos of the latitude (for speed optimization)
int find_lowest_neighbor(int row, int col, Model<double>* DEM_filled_no_flat, double coslat)
{
	int result = 0;
	double min_drop = 0;
	double min_dist = 100000;
	for (uint d=0; d<directions.size(); d++) {
		int row_neighbor = row+directions[d].row;
		int col_neighbor = col+directions[d].col;
		if (DEM_filled_no_flat->check_within(row_neighbor, col_neighbor)) {
			double drop = DEM_filled_no_flat->get(row_neighbor,col_neighbor)-DEM_filled_no_flat->get(row,col);
			if(drop>=0)
				continue;
			double dist = find_distance(DEM_filled_no_flat->get_coordinate(row, col), DEM_filled_no_flat->get_coordinate(row_neighbor, col_neighbor), coslat);
			if (drop*min_dist < min_drop*dist) {
				min_drop = drop;
				min_dist = dist;
				result = d;
			}
		}
	}
	if(min_drop==0)
		search_config.logger.debug("Alert: Minimum drop of 0 at " + to_string(row) + " " + to_string(col));
	return result;
}

// Find the direction of flow for each square in a filled DEM
static Model<char>* flow_direction(Model<double>* DEM_filled_no_flat)
{
	Model<char>* flow_dirn = new Model<char>(DEM_filled_no_flat->nrows(), DEM_filled_no_flat->ncols(), MODEL_UNSET);
	flow_dirn->set_geodata(DEM_filled_no_flat->get_geodata());
	double coslat = COS(RADIANS(flow_dirn->get_origin().lat-(0.5+border/(double)(flow_dirn->nrows()-2*border))));
	for (int row=1; row<flow_dirn->nrows()-1;row++)
		for (int col=1; col<flow_dirn->ncols()-1; col++)
			flow_dirn->set(row,col,find_lowest_neighbor(row, col, DEM_filled_no_flat, coslat));

	for (int row=0; row<flow_dirn->nrows()-1;row++) {
		flow_dirn->set(row,0,4);
		flow_dirn->set(flow_dirn->nrows()-row-1,flow_dirn->ncols()-1,0);
	}
	for (int col=0; col<flow_dirn->ncols()-1; col++) {
 		flow_dirn->set(0,col+1,6);
		flow_dirn->set(flow_dirn->nrows()-1,flow_dirn->ncols()-col-2,2);
	}
	flow_dirn->set(0,0,5);
	flow_dirn->set(0,flow_dirn->ncols()-1,7);
	flow_dirn->set(flow_dirn->nrows()-1,flow_dirn->ncols()-1,1);
	flow_dirn->set(flow_dirn->nrows()-1,0,3);
	return flow_dirn;
}

// Calculate flow accumulation given a DEM and the flow directions
static Model<int>* find_flow_accumulation(Model<char>* flow_directions, Model<double>* DEM_filled_no_flat)
{
	Model<int>* flow_accumulation = new Model<int>(DEM_filled_no_flat->nrows(), DEM_filled_no_flat->ncols(), MODEL_SET_ZERO);
	flow_accumulation->set_geodata(DEM_filled_no_flat->get_geodata());

	ArrayCoordinateWithHeight* to_check = new ArrayCoordinateWithHeight[flow_accumulation->nrows()*flow_accumulation->ncols()];

	for (int row=0; row<flow_accumulation->nrows(); row++)
		for (int col=0; col<flow_accumulation->ncols();col++) {
			to_check[DEM_filled_no_flat->ncols()*row+col].col = col;
			to_check[DEM_filled_no_flat->ncols()*row+col].row = row;
			to_check[DEM_filled_no_flat->ncols()*row+col].h = DEM_filled_no_flat->get(row,col);
		}

	// start at the highest point and distribute flow downstream
	sort(to_check, to_check+flow_accumulation->nrows()*flow_accumulation->ncols());

	for (int i=0; i<DEM_filled_no_flat->nrows()*DEM_filled_no_flat->ncols();i++) {
		ArrayCoordinateWithHeight p = to_check[i];
		ArrayCoordinateWithHeight downstream = ArrayCoordinateWithHeight_init(p.row+directions[flow_directions->get(p.row,p.col)].row, p.col+directions[flow_directions->get(p.row,p.col)].col,0);
		if (flow_accumulation->check_within( downstream.row, downstream.col) ){
			//printf("%4d %4d %3d %3d %2d %2d %1d\n", downstream.row,downstream.col,flow_accumulation->get(p.row,p.col), flow_accumulation->get(p.row,p.col)+1, directions[flow_directions->get(p.row,p.col)].row, directions[flow_directions->get(p.row,p.col)].col, flow_directions->get(p.row,p.col));
			flow_accumulation->set(downstream.row,downstream.col, flow_accumulation->get(p.row,p.col)+flow_accumulation->get(downstream.row,downstream.col)+1);
		}
	}
	delete[] to_check;
	return flow_accumulation;
}

// Find streams given the flow accumulation
static Model<bool>* find_streams(Model<int>* flow_accumulation)
{
	Model<bool>* streams = new Model<bool>(flow_accumulation->nrows(), flow_accumulation->ncols(), MODEL_SET_ZERO);
	streams->set_geodata(flow_accumulation->get_geodata());
	int stream_site_count=0;
	for (int row=0; row<flow_accumulation->nrows(); row++)
		for (int col=0; col<flow_accumulation->ncols();col++)
			if(flow_accumulation->get(row,col) >= stream_threshold){
				streams->set(row,col,true);
				stream_site_count++;
			}
	search_config.logger.debug("Number of stream sites = "+  to_string(stream_site_count));
	return streams;
}


// Find dam sites to check given the streams, flow directions and DEM
static Model<bool>* find_pour_points(Model<bool>* streams, Model<char>* flow_directions, Model<short>* DEM_filled)
{
	Model<bool>* pour_points = new Model<bool>(streams->nrows(), streams->ncols(), MODEL_SET_ZERO);
	pour_points->set_geodata(streams->get_geodata());
	int pour_point_count=0;
	for (int row = border; row < border+pour_points->nrows()-2*border; row++)
		for (int col = border; col <  border+pour_points->ncols()-2*border; col++)
			if (streams->get(row,col)) {
				ArrayCoordinate downstream = ArrayCoordinate_init(row+directions[flow_directions->get(row,col)].row,col+directions[flow_directions->get(row,col)].col, GeographicCoordinate_init(0,0));
        if ( flow_directions->check_within(downstream.row, downstream.col)){
          if(DEM_filled->get(row,col) >= 0){
            if(DEM_filled->get(row,col)-DEM_filled->get(row,col)%contour_height>DEM_filled->get(downstream.row,downstream.col)) {
              pour_points->set(row,col,true);
              pour_point_count++;
            }
          } else {
            if(DEM_filled->get(row,col)+DEM_filled->get(row,col)%contour_height>DEM_filled->get(downstream.row,downstream.col)) {
              pour_points->set(row,col,true);
              pour_point_count++;
            }
          }
        }
			}
	search_config.logger.debug("Number of dam sites = "+  to_string(pour_point_count));
	return pour_points;
}

// Find details of possible reservoirs at pour_point
static RoughGreenfieldReservoir model_greenfield_reservoir(ArrayCoordinate pour_point, Model<char>* flow_directions, Model<short>* DEM_filled, Model<bool>* filter,
				  Model<int>* modelling_array, int iterator)
{

	RoughGreenfieldReservoir reservoir = RoughGreenfieldReservoir(RoughReservoir(pour_point, convert_to_int(DEM_filled->get(pour_point.row,pour_point.col))));

  double area_at_elevation[max_wall_height + 1];
  double cumulative_area_at_elevation[max_wall_height + 1];
  double volume_at_elevation[max_wall_height + 1];
  double dam_length_at_elevation[max_wall_height + 1];
  std::memset(area_at_elevation, 0, (max_wall_height+1)*sizeof(double));
  std::memset(volume_at_elevation, 0, (max_wall_height+1)*sizeof(double));
  std::memset(cumulative_area_at_elevation, 0, (max_wall_height+1)*sizeof(double));
  std::memset(dam_length_at_elevation, 0, (max_wall_height+1)*sizeof(double));

	queue<ArrayCoordinate> q;
	q.push(pour_point);
	while (!q.empty()) {
		ArrayCoordinate p = q.front();
		q.pop();

		int elevation = convert_to_int(DEM_filled->get(p.row,p.col));
		int elevation_above_pp = MAX(elevation - reservoir.elevation, 0);

		update_reservoir_boundary(reservoir.shape_bound, p, elevation_above_pp);

		if (filter->get(p.row,p.col))
			reservoir.max_dam_height = MIN(reservoir.max_dam_height,elevation_above_pp);

		area_at_elevation[elevation_above_pp+1] += find_area(p);
		modelling_array->set(p.row,p.col,iterator);

		for (uint d=0; d<directions.size(); d++) {
			ArrayCoordinate neighbor = {p.row+directions[d].row, p.col+directions[d].col, p.origin};
			if (flow_directions->check_within(neighbor.row, neighbor.col) &&
			    flow_directions->flows_to(neighbor, p) &&
			    (convert_to_int(DEM_filled->get(neighbor.row,neighbor.col)-DEM_filled->get(pour_point.row,pour_point.col)) < max_wall_height) ) {
				q.push(neighbor);
			}
		}
	}

	for (int ih=1; ih<max_wall_height+1;ih++) {
		cumulative_area_at_elevation[ih] = cumulative_area_at_elevation[ih-1] + area_at_elevation[ih];
		volume_at_elevation[ih] = volume_at_elevation[ih-1] + 0.01*cumulative_area_at_elevation[ih]; // area in ha, vol in GL
	}

	q.push(pour_point);
	while (!q.empty()) {
		ArrayCoordinate p = q.front();
		q.pop();
		int elevation = convert_to_int(DEM_filled->get(p.row,p.col));
		int elevation_above_pp = MAX(elevation - reservoir.elevation,0);
		for (uint d=0; d<directions.size(); d++) {
			ArrayCoordinate neighbor = {p.row+directions[d].row, p.col+directions[d].col, p.origin};
			if (flow_directions->check_within(neighbor.row, neighbor.col)){
				if(flow_directions->flows_to(neighbor, p) &&
          (convert_to_int(DEM_filled->get(neighbor.row,neighbor.col)-DEM_filled->get(pour_point.row,pour_point.col)) < max_wall_height) ) {
					q.push(neighbor);
				}
				if ((directions[d].row * directions[d].col == 0) // coordinate orthogonal directions
				    && (modelling_array->get(neighbor.row,neighbor.col) < iterator ) ){
					dam_length_at_elevation[MIN(MAX(elevation_above_pp, convert_to_int(DEM_filled->get(neighbor.row,neighbor.col)-reservoir.elevation)),max_wall_height)] +=find_orthogonal_nn_distance(p, neighbor);	//WE HAVE PROBLEM IF VALUE IS NEGATIVE???
				}
			}
		}
	}

	for (uint ih =0 ; ih< dam_wall_heights.size(); ih++) {
		int height = dam_wall_heights[ih];
		reservoir.areas.push_back(cumulative_area_at_elevation[height]);
		reservoir.dam_volumes.push_back(0);
		for (int jh=0; jh < height; jh++)
			reservoir.dam_volumes[ih] += convert_to_dam_volume(height-jh, dam_length_at_elevation[jh]);
		reservoir.volumes.push_back(volume_at_elevation[height] + 0.5*reservoir.dam_volumes[ih]);
		reservoir.water_rocks.push_back(reservoir.volumes[ih]/reservoir.dam_volumes[ih]);
	}

	return reservoir;
}

static int
model_reservoirs(GridSquare square_coordinate, Model<bool> *pour_points,
                 Model<char> *flow_directions, Model<short> *DEM_filled,
                 Model<int> *flow_accumulation, Model<bool> *filter) {
  FILE *csv_file;
  // Open the appropriate CSV file based on the search type
  if (search_config.search_type == SearchType::OCEAN)
    csv_file = fopen(convert_string(file_storage_location +
                                    "output/reservoirs/ocean_" +
                                    str(square_coordinate) + "_reservoirs.csv"),
                     "w");
  else
    csv_file =
        fopen(convert_string(file_storage_location + "output/reservoirs/" +
                             str(square_coordinate) + "_reservoirs.csv"),
              "w");
  // Check if the CSV file was opened successfully
  if (!csv_file) {
    cout << "Failed to open reservoir CSV file" << endl;
    exit(1);
  }
  // Write the header for the CSV file
  write_rough_reservoir_csv_header(csv_file);

  FILE *csv_data_file;
  // Open the appropriate CSV data file based on the search type
  if (search_config.search_type == SearchType::OCEAN)
    csv_data_file =
        fopen(convert_string(file_storage_location +
                             "processing_files/reservoirs/ocean_" +
                             str(square_coordinate) + "_reservoirs_data.csv"),
              "w");
  else
    csv_data_file = fopen(
        convert_string(file_storage_location + "processing_files/reservoirs/" +
                       str(square_coordinate) + "_reservoirs_data.csv"),
        "w");
  // Check if the CSV data file was opened successfully
  if (!csv_file) {
    fprintf(stderr, "failed to open reservoir CSV data file\n");
    exit(1);
  }
  // Write the header for the CSV data file
  write_rough_reservoir_data_header(csv_data_file);

  int i = 0;
  int count = 0;
  // Create a new model for storing reservoir data
  Model<int> *model = new Model<int>(pour_points->nrows(), pour_points->ncols(),
                                     MODEL_SET_ZERO);

  // If the search type is OCEAN, find the appropriate pour points and model reservoirs
  if (search_config.search_type == SearchType::OCEAN) {
    unique_ptr<ArrayCoordinate> pp(new ArrayCoordinate{-1, -1, get_origin(square_coordinate, border)});
    for (int row = border + 1; row < border + DEM_filled->nrows() - 2 * border - 1; row++)
      for (int col = border + 1; col < border + DEM_filled->ncols() - 2 * border - 1; col++) {
        if (filter->get(row, col))
          continue;
        if (DEM_filled->get(row, col) >= 1 - EPS &&
            pour_points->get(row + directions.at(flow_directions->get(row, col)).row,
                             col + directions.at(flow_directions->get(row, col)).col) == true) {
          pp->row = row;
          pp->col = col;
        }
      }
    // If a valid pour point is found, create a RoughBfieldReservoir and write to CSV
    if (pp->row>0) {
      RoughBfieldReservoir reservoir = RoughBfieldReservoir(RoughReservoir(*pp, 0));
      reservoir.identifier = str(square_coordinate) + "_OCEAN";
      reservoir.ocean = true;
      reservoir.watershed_area = 0;
      reservoir.max_dam_height = 0;
      for (uint ih = 0; ih < dam_wall_heights.size(); ih++) {
        reservoir.areas.push_back(0);
        reservoir.dam_volumes.push_back(0);
        reservoir.volumes.push_back(INF);
        reservoir.water_rocks.push_back(INF);
      }
      for (int row = border + 1; row < border + DEM_filled->nrows() - 2 * border - 1; row++)
        for (int col = border + 1; col < border + DEM_filled->ncols() - 2 * border - 1; col++) {
          if (filter->get(row, col))
            continue;
          if (DEM_filled->get(row, col) >= 1 - EPS &&
              pour_points->get(row + directions.at(flow_directions->get(row, col)).row,
                               col + directions.at(flow_directions->get(row, col)).col) == true) {
            ArrayCoordinate pour_point = {row, col, get_origin(square_coordinate, border)};
            reservoir.shape_bound.push_back(pour_point);
            count++;
          }
        }
      write_rough_reservoir_csv(csv_file, reservoir);
      write_rough_reservoir_data(csv_data_file, &reservoir);
    }
  } else {
    // For non-OCEAN search types, find the appropriate pour points and model reservoirs
    for (int row = border; row < border + DEM_filled->nrows() - 2 * border; row++)
      for (int col = border; col < border + DEM_filled->ncols() - 2 * border; col++) {
        if (!pour_points->get(row, col) || filter->get(row, col))
          continue;
        ArrayCoordinate pour_point = {row, col, get_origin(square_coordinate, border)};
        i++;
        RoughGreenfieldReservoir reservoir =
            model_greenfield_reservoir(pour_point, flow_directions, DEM_filled, filter, model, i);
        reservoir.ocean = false;
        if (max(reservoir.volumes) >= min_reservoir_volume &&
            max(reservoir.water_rocks) > min_reservoir_water_rock &&
            reservoir.max_dam_height >= min_max_dam_height) {
          reservoir.watershed_area = find_area(pour_point) * flow_accumulation->get(row, col);

          reservoir.identifier = str(square_coordinate) + "_RES" + str(i);

          write_rough_reservoir_csv(csv_file, reservoir);
          write_rough_reservoir_data(csv_data_file, &reservoir);
          count++;
        }
      }
  }
  // Close the CSV files
  fclose(csv_file);
  fclose(csv_data_file);
  return count;
  }

int main(int nargs, char **argv) {
  // Initialize search configuration with command line arguments
  search_config = SearchConfig(nargs, argv);
  cout << "Screening started for " << search_config.filename() << endl;

  // Register GDAL drivers
  GDALAllRegister();
  // Parse variables from configuration files
  parse_variables(convert_string("storage_location"));
  parse_variables(convert_string(file_storage_location + "variables"));
  unsigned long start_usec = walltime_usec();
  unsigned long t_usec = start_usec;

  // Create necessary directories for output and processing files
  mkdir(convert_string(file_storage_location + "output"), 0777);
  mkdir(convert_string(file_storage_location + "output/reservoirs"), 0777);
  mkdir(convert_string(file_storage_location + "processing_files"), 0777);
  mkdir(convert_string(file_storage_location + "processing_files/reservoirs"), 0777);

  // Check if the search type is not existing
  if (search_config.search_type.not_existing()) {
    // Read DEM with borders
    Model<short> *DEM = read_DEM_with_borders(search_config.grid_square, border);

    // Output debug information if enabled
    if (search_config.logger.output_debug()) {
      printf("\nAfter border added:\n");
      DEM->print();
    }
    if (debug_output) {
      mkdir(convert_string("debug"), 0777);
      mkdir(convert_string("debug/input"), 0777);
      DEM->write("debug/input/" + str(search_config.grid_square) + "_input.tif", GDT_Int16);
    }

    Model<char> *flow_directions;
    Model<bool> *pour_points;
    Model<int> *flow_accumulation;
    Model<short> *DEM_filled;
    Model<bool> *filter;

    //filter = new Model<bool>(file_storage_location+"debug/filter/"+str(search_config.grid_square)+"_filter.tif", GDT_Byte);
    //DEM_filled = new Model<short>(file_storage_location+"debug/DEM_filled/"+str(search_config.grid_square)+"_DEM_filled.tif", GDT_Int16);
    //flow_directions = new Model<char>(file_storage_location+"debug/flow_directions/"+str(search_config.grid_square)+"_flow_directions.tif", GDT_Byte);
    //flow_accumulation =  new Model<int>(file_storage_location+"debug/flow_accumulation/"+str(search_config.grid_square)+"_flow_accumulation.tif", GDT_Int32);
    //pour_points = new Model<bool>(file_storage_location+"debug/pour_points/"+str(search_config.grid_square)+"_pour_points.tif", GDT_Byte);
    t_usec = walltime_usec();
    filter = read_filter(DEM, filter_filenames);
    if (search_config.logger.output_debug()) {
      printf("\nFilter:\n");
      filter->print();
      printf("Filter Runtime: %.2f sec\n", 1.0e-6*(walltime_usec() - t_usec) );
    }
    if(debug_output){
      mkdir(convert_string(file_storage_location+"debug/filter"),0777);
      filter->write(file_storage_location+"debug/filter/"+str(search_config.grid_square)+"_filter.tif", GDT_Byte);
    }

    // Fill DEM and convert to short model
    t_usec = walltime_usec();
    Model<double>* DEM_filled_no_flat = fill(DEM);
    DEM_filled = new Model<short>(DEM->nrows(), DEM->ncols(), MODEL_SET_ZERO);
    DEM_filled->set_geodata(DEM->get_geodata());
    for(int row = 0; row<DEM->nrows();row++)
      for(int col = 0; col<DEM->ncols();col++)
        DEM_filled->set(row, col, convert_to_int(DEM_filled_no_flat->get(row, col)));
    if (search_config.logger.output_debug()) {
      printf("\nFilled No Flats:\n");
      DEM_filled_no_flat->print();
      printf("Fill Runtime: %.2f sec\n", 1.0e-6*(walltime_usec() - t_usec) );
    }
    if(debug_output){
      mkdir(convert_string(file_storage_location+"debug/DEM_filled"),0777);
      DEM_filled->write(file_storage_location+"debug/DEM_filled/"+str(search_config.grid_square)+"_DEM_filled.tif", GDT_Int16);
      DEM_filled_no_flat->write(file_storage_location+"debug/DEM_filled/"+str(search_config.grid_square)+"_DEM_filled_no_flat.tif",GDT_Float64);
    }

    // Calculate flow directions
    t_usec = walltime_usec();
    flow_directions = flow_direction(DEM_filled_no_flat);
    if (search_config.logger.output_debug()) {
      printf("\nFlow Directions:\n");
      flow_directions->print();
      printf("Flow directions Runtime: %.2f sec\n", 1.0e-6*(walltime_usec() - t_usec) );
    }
    if(debug_output){
      mkdir(convert_string(file_storage_location+"debug/flow_directions"),0777);
      flow_directions->write(file_storage_location+"debug/flow_directions/"+str(search_config.grid_square)+"_flow_directions.tif",GDT_Byte);
    }
    mkdir(convert_string(file_storage_location+"processing_files/flow_directions"),0777);
    flow_directions->write(file_storage_location+"processing_files/flow_directions/"+str(search_config.grid_square)+"_flow_directions.tif",GDT_Byte);

    // Calculate flow accumulation
    t_usec = walltime_usec();
    flow_accumulation = find_flow_accumulation(flow_directions, DEM_filled_no_flat);
    if (search_config.logger.output_debug()) {
      printf("\nFlow Accumulation:\n");
      flow_accumulation->print();
      printf("Flow accumulation Runtime: %.2f sec\n", 1.0e-6*(walltime_usec() - t_usec) );
    }
    if(debug_output){
      mkdir(convert_string(file_storage_location+"debug/flow_accumulation"),0777);
      flow_accumulation->write(file_storage_location+"debug/flow_accumulation/"+str(search_config.grid_square)+"_flow_accumulation.tif", GDT_Int32);
    }
    delete DEM_filled_no_flat;

    // Find pour points based on search type
    if(search_config.search_type == SearchType::OCEAN){
      pour_points = find_ocean(DEM);
      if (search_config.logger.output_debug()) {
        printf("\nOcean\n");
        pour_points->print();
      }
      if(debug_output){
        mkdir(convert_string(file_storage_location+"debug/ocean"),0777);
        pour_points->write(file_storage_location+"debug/ocean/"+str(search_config.grid_square)+"_ocean.tif",GDT_Byte);
      }
    }else{


      Model<bool>* streams = find_streams(flow_accumulation);
      if (search_config.logger.output_debug()) {
        printf("\nStreams (Greater than %d accumulation):\n", stream_threshold);
        streams->print();
      }
      if(debug_output){
        mkdir(convert_string(file_storage_location+"debug/streams"),0777);
        streams->write(file_storage_location+"debug/streams/"+str(search_config.grid_square)+"_streams.tif",GDT_Byte);
      }

      pour_points = find_pour_points(streams, flow_directions, DEM_filled);
      if (search_config.logger.output_debug()) {
        printf("\nPour points (Streams every %dm):\n", contour_height);
        pour_points->print();
      }
      if(debug_output){
        mkdir(convert_string(file_storage_location+"debug/pour_points"),0777);
        pour_points->write(file_storage_location+"debug/pour_points/"+str(search_config.grid_square)+"_pour_points.tif",GDT_Byte);
      }
      delete streams;
    }

		t_usec = walltime_usec();
		int count = model_reservoirs(search_config.grid_square, pour_points, flow_directions, DEM_filled, flow_accumulation, filter);
		search_config.logger.debug("Found " + to_string(count) + " reservoirs. Runtime: " + to_string(1.0e-6*(walltime_usec() - t_usec)) + " sec");
		printf(convert_string("Screening finished for "+search_config.search_type.prefix()+str(search_config.grid_square)+". Runtime: %.2f sec\n"), 1.0e-6*(walltime_usec() - start_usec) );
  } else {
	// Depression volume finding for pits
	if (search_config.search_type == SearchType::BULK_PIT) {
		t_usec = walltime_usec();
		Model<short> *DEM = read_DEM_with_borders(search_config.grid_square, border);
		Model<double>* DEM_filled_no_flat = fill(DEM);
		Model<short>* DEM_filled = new Model<short>(DEM->nrows(), DEM->ncols(), MODEL_SET_ZERO);
		DEM_filled->set_geodata(DEM->get_geodata());
		for(int row = 0; row<DEM->nrows();row++) {
			for(int col = 0; col<DEM->ncols();col++) {
				DEM_filled->set(row, col, convert_to_int(DEM_filled_no_flat->get(row, col)));
			}
		}

		depression_volume_finding(DEM_filled);
		printf(convert_string("Volume finding finished for "+str(search_config.grid_square)+". Runtime: %.2f sec\n"), 1.0e-6*(walltime_usec() - t_usec) );
	}

    // Open CSV files for output
    FILE *csv_file = fopen(convert_string(file_storage_location + "output/reservoirs/" +
                                          search_config.filename() + "_reservoirs.csv"),
                           "w");
    if (!csv_file) {
      cout << "Failed to open reservoir CSV file." << endl;
      exit(1);
    }

    write_rough_reservoir_csv_header(csv_file);

    FILE *csv_data_file =
        fopen(convert_string(file_storage_location + "processing_files/reservoirs/" +
              search_config.filename() +
                             "_reservoirs_data.csv"),
              "w");
    if (!csv_file) {
      fprintf(stderr, "failed to open reservoir CSV data file\n");
      exit(1);
    }
    write_rough_reservoir_data_header(csv_data_file);

    vector<ExistingReservoir> existing_reservoirs;

    // Get existing reservoirs based on search type
    if(search_config.search_type.single())
      existing_reservoirs.push_back(get_existing_reservoir(search_config.name));
    else
      existing_reservoirs = get_existing_reservoirs(search_config.grid_square);

    if (existing_reservoirs.size() < 1) {
      printf("No existing reservoirs in %s\n", convert_string(str(search_config.grid_square)));
      exit(0);
    }
    if (search_config.search_type == SearchType::BULK_EXISTING && use_tiled_rivers) {
      Model<short> *DEM = read_DEM_with_borders(search_config.grid_square, border);
      Model<double> *DEM_filled_no_flat = fill(DEM);
      for (ExistingReservoir r : existing_reservoirs) {
        RoughBfieldReservoir reservoir = existing_reservoir_to_rough_reservoir(r);
        reservoir.pit = (search_config.search_type == SearchType::BULK_PIT ||
                         search_config.search_type == SearchType::SINGLE_PIT);
        if (reservoir.river) {
          for (ArrayCoordinate ac : reservoir.shape_bound) {
            int temp_elevation = INT_MAX;
            for (int dx = -5; dx < 6; dx++)
              for (int dy = -5; dy < 6; dy++) {
                ArrayCoordinate n = ArrayCoordinate_init(ac.row + dy, ac.col + dx, ac.origin);
                if (DEM_filled_no_flat->check_within(n.row, n.col))
                  temp_elevation =
                      MIN(temp_elevation,
                          (int)DEM_filled_no_flat->get(convert_coordinates(n)));
              }
            reservoir.elevations.push_back(temp_elevation);
          }
          reservoir.elevation = reservoir.elevations[0];
        }
        write_rough_reservoir_csv(csv_file, reservoir);
        write_rough_reservoir_data(csv_data_file, &reservoir);
      }
    } else {
      for (ExistingReservoir r : existing_reservoirs) {
        RoughBfieldReservoir reservoir = existing_reservoir_to_rough_reservoir(r);
        reservoir.pit = (search_config.search_type == SearchType::BULK_PIT ||
                         search_config.search_type == SearchType::SINGLE_PIT);
        write_rough_reservoir_csv(csv_file, reservoir);
        write_rough_reservoir_data(csv_data_file, &reservoir);
      }
    }

    // Close CSV files
    fclose(csv_file);
    fclose(csv_data_file);
    printf(convert_string("Screening finished for " + search_config.filename() +
                          ". Runtime: %.2f sec\n"),
           1.0e-6 * (walltime_usec() - start_usec));
  }
}
