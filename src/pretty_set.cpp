#include "phes_base.h"
#include "search_config.hpp"
#include "constructor_helpers.hpp"

// Function to check if a pair of reservoirs is valid based on various conditions
bool check_pair(Pair &pair, Model<bool> *seen, BigModel &big_model, set<string>& used_with_river) {
  vector<vector<vector<GeographicCoordinate>>> empty_countries;
  vector<string> empty_country_names;
  vector<ArrayCoordinate> used_points;
  // Check if the lower reservoir is a river and has already been used with the upper reservoir
  if(pair.lower.river && used_with_river.contains(pair.upper.identifier))
    return false;
  // Check if the lower reservoir is a river and tiled rivers are not used
  if(pair.lower.river && !use_tiled_rivers)
    return false;
  // Check if the upper reservoir is not a brownfield and cannot be modeled
  if (!pair.upper.brownfield &&
      !model_reservoir(&pair.upper, NULL, seen, NULL, &used_points, big_model, NULL,
                       empty_countries, empty_country_names))
    return false;
  // Check if the lower reservoir is not a brownfield or ocean and cannot be modeled
  if (!pair.lower.brownfield && !pair.lower.ocean &&
      !model_reservoir(&pair.lower, NULL, seen, NULL, &used_points, big_model, NULL,
                       empty_countries, empty_country_names))
    return false;

  // Check if the upper reservoir is a brownfield with a large volume and the lower reservoir is not a brownfield
  if (pair.upper.brownfield && pair.upper.volume > INF/10 && !pair.lower.brownfield) {
    if (pair.lower.area > max_bluefield_surface_area_ratio * pair.upper.area)
      return false;
  }
  // Check if the lower reservoir is a brownfield with a large volume and the upper reservoir is not a brownfield
  if (pair.lower.brownfield && pair.lower.volume > INF/10 && !pair.upper.brownfield) {
    if (pair.upper.area > max_bluefield_surface_area_ratio * pair.lower.area)
      return false;
  }

  // Mark the used points in the seen model
  for (uint i = 0; i < used_points.size(); i++) {
    seen->set(used_points[i].row, used_points[i].col, true);
  }
  // Add the upper reservoir identifier to the used_with_river set if the lower reservoir is a river
  if(pair.lower.river)
    used_with_river.insert(pair.upper.identifier);

  return true;
}

int main(int nargs, char **argv)
{
  // Initialize search configuration
  search_config = SearchConfig(nargs, argv);
  vector<vector<Pair>> pairs;
  set<string> used_with_river;

	cout << "Pretty set started for " << search_config.filename() << endl;

	GDALAllRegister();
	parse_variables(convert_string("storage_location"));
	parse_variables(convert_string(file_storage_location+"variables"));
	unsigned long t_usec = walltime_usec();

	pairs = read_rough_pair_data(convert_string(file_storage_location+"processing_files/pairs/"+search_config.filename()+"_rough_pairs_data.csv"));

	mkdir(convert_string(file_storage_location+"processing_files/pretty_set_pairs"),0777);
	FILE *csv_data_file = fopen(convert_string(file_storage_location+"processing_files/pretty_set_pairs/"+search_config.filename()+"_rough_pretty_set_pairs_data.csv"), "w");
	write_rough_pair_data_header(csv_data_file);

	uint total_pairs = 0;
	for(uint i = 0; i<pairs.size(); i++)
		total_pairs += pairs[i].size();

	if (total_pairs == 0) {
		cout << "No pairs found" << endl;
		cout << "Pretty set finished for " << search_config.filename() << ". Runtime: " << 1.0e-6*(walltime_usec() - t_usec)<< " sec" << endl;
		return 0;
	}

	if(search_config.search_type.single())
		search_config.grid_square = get_square_coordinate(get_existing_reservoir(search_config.name));

	BigModel big_model = BigModel_init(search_config.grid_square);

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
			if(check_pair(pairs[i][j], seen, big_model, used_with_river)){
				write_rough_pair_data(csv_data_file, &pairs[i][j]);
				count++;
			}
		}
		delete seen;
		search_config.logger.debug(to_string(count) + " " + to_string(tests[i].energy_capacity) + "GWh "+to_string(tests[i].storage_time) + "h Pairs");
	}
	cout << "Pretty set finished for " << search_config.filename() << ". Runtime: " << 1.0e-6*(walltime_usec() - t_usec)<< " sec" << endl;
}
