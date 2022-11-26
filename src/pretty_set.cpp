#include "phes_base.h"
#include "search_config.hpp"

SearchConfig search_config;

vector<vector<Pair>> pairs;

bool check_reservoir(Reservoir& reservoir, Model<bool>* seen, vector<ArrayCoordinate> *used_points, BigModel& big_model){
	Model<short>* DEM = big_model.DEM;
	Model<char>* flow_directions = big_model.flow_directions[0];

	for(int i = 0; i<9; i++)
		if(big_model.neighbors[i].lat == convert_to_int(FLOOR(reservoir.latitude+EPS)) && big_model.neighbors[i].lon == convert_to_int(FLOOR(reservoir.longitude+EPS)))
			flow_directions = big_model.flow_directions[i];

	ArrayCoordinate offset = convert_coordinates(convert_coordinates(ArrayCoordinate_init(0,0,flow_directions->get_origin())), DEM->get_origin());

	double volume = 0;
	double req_volume = reservoir.volume;
	double wall_height = reservoir.dam_height;
	vector<ArrayCoordinate> temp_used_points;

	char last = 'd';

	while(volume*(1+0.5/reservoir.water_rock)<(1-volume_accuracy)*req_volume || volume*(1+0.5/reservoir.water_rock)>(1+volume_accuracy)*req_volume){
		temp_used_points.clear();
		volume = 0;
		queue<ArrayCoordinate> q;
		q.push(reservoir.pour_point);
		ArrayCoordinate reservoir_big_ac = convert_coordinates(convert_coordinates(reservoir.pour_point), DEM->get_origin());
		ArrayCoordinate neighbor;
		// printf("%d %d %d %d\n", reservoir_big_ac.row, reservoir_big_ac.col, reservoir.pour_point.row, reservoir.pour_point.col);
		while (!q.empty()) {
			ArrayCoordinate p = q.front();
			q.pop();

			ArrayCoordinate big_ac = {p.row+offset.row, p.col+offset.col, DEM->get_origin()};

			temp_used_points.push_back(big_ac);

			if (seen->get(big_ac.row,big_ac.col)||DEM->get(big_ac.row, big_ac.col)<-2000)
				return false;

			volume+=(wall_height-(DEM->get(big_ac.row,big_ac.col)-DEM->get(reservoir_big_ac.row,reservoir_big_ac.col)))*find_area(p)/100; // Minor optimization with coslat?
			// if(wall_height<0)
			// 	printf("%d %d %f %f %d %d %f\n", big_ac.row, big_ac.col, wall_height, (wall_height-(DEM->get(big_ac.row,big_ac.col)-DEM->get(reservoir_big_ac.row,reservoir_big_ac.col)))*find_area(p)/100, DEM->get(big_ac.row,big_ac.col), DEM->get(reservoir_big_ac.row,reservoir_big_ac.col), find_area(p));
			for (uint d=0; d<directions.size(); d++) {
				neighbor = {p.row+directions[d].row, p.col+directions[d].col, p.origin};
				if (flow_directions->check_within(neighbor.row, neighbor.col) &&
				    flows_to(neighbor, p, flow_directions) &&
				    ((DEM->get(neighbor.row+offset.row, neighbor.col+offset.col)-DEM->get(reservoir_big_ac.row,reservoir_big_ac.col)) < wall_height) ) {
					q.push(neighbor);
				}
			}
		}
		// printf("%f %f %d\n", volume, wall_height, DEM->get(reservoir_big_ac.row, reservoir_big_ac.col));
		if(volume*(1+0.5/reservoir.water_rock)<(1-volume_accuracy)*req_volume){
			wall_height+=dam_wall_height_resolution;
            if(wall_height>reservoir.max_dam_height)
                return false;
            last = 'u';
		}
                
        if(volume*(1+0.5/reservoir.water_rock)>(1+volume_accuracy)*req_volume){
        	if (last == 'u')
                return false;
            wall_height-=dam_wall_height_resolution;
            last = 'd';
        }            
	}

	if(wall_height<minimum_dam_height){
        return false;
    }
	for(uint i = 0; i<temp_used_points.size(); i++){
		used_points->push_back(temp_used_points[i]);
	}
	return true;
}

bool check_pair(Pair& pair, Model<bool>* seen, BigModel& big_model){
	vector<ArrayCoordinate> used_points;
	if(!pair.upper.brownfield && !check_reservoir(pair.upper, seen, &used_points, big_model))
		return false;
	if(!pair.lower.brownfield && !pair.lower.ocean && !check_reservoir(pair.lower, seen, &used_points, big_model))
		return false;

	for(uint i = 0; i<used_points.size();i++){
		seen->set(used_points[i].row,used_points[i].col,true);
	}

	return true;
}

int main(int nargs, char **argv)
{
  search_config = SearchConfig(nargs, argv);

	cout << "Pretty set started for " << search_config.filename() << endl;

	GDALAllRegister();
	parse_variables(convert_string("storage_location"));
	parse_variables(convert_string(file_storage_location+"variables"));
	unsigned long t_usec = walltime_usec();
	
	if(search_config.search_type.single())
		search_config.grid_square = get_square_coordinate(get_existing_reservoir(search_config.name));
	
	BigModel big_model = BigModel_init(search_config.grid_square);

	pairs = read_rough_pair_data(convert_string(file_storage_location+"processing_files/pairs/"+search_config.filename()+"_rough_pairs_data.csv"));

	mkdir(convert_string(file_storage_location+"processing_files/pretty_set_pairs"),0777);
	FILE *csv_data_file = fopen(convert_string(file_storage_location+"processing_files/pretty_set_pairs/"+search_config.filename()+"_rough_pretty_set_pairs_data.csv"), "w");
	write_rough_pair_data_header(csv_data_file);

	for(uint i = 0; i<tests.size(); i++){
		sort(pairs[i].begin(), pairs[i].end());
		Model<bool>* seen = new Model<bool>(big_model.DEM->nrows(), big_model.DEM->nrows(), MODEL_SET_ZERO);
		seen->set_geodata(big_model.DEM->get_geodata());

		if(search_config.search_type.single()){
			ExistingReservoir r = get_existing_reservoir(search_config.name);
			polygon_to_raster(r.polygon, seen);
		}

		int count = 0;
		for(uint j=0; j<pairs[i].size(); j++){
			if(check_pair(pairs[i][j], seen, big_model)){
				write_rough_pair_data(csv_data_file, &pairs[i][j]);
				count++;
			}
		}
		delete seen;
		search_config.logger.debug(to_string(count) + " " + to_string(tests[i].energy_capacity) + "GWh "+to_string(tests[i].storage_time) + "h Pairs");
	}
	cout << "Pretty set finished for " << search_config.filename() << ". Runtime: " << 1.0e-6*(walltime_usec() - t_usec)<< " sec" << endl;
}
