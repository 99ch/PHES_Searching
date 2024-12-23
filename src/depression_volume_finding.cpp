#include "phes_base.h"

int main(int nargs, char **argv)
{
  // Check if the number of arguments is less than 3
  if(nargs < 3){
    cout << "Not enough arguements" << endl;
    return -1;
  }

  // Initialize the grid square coordinates
  GridSquare square_coordinate = GridSquare_init(atoi(argv[2]), atoi(argv[1]));
  search_config.logger = Logger::DEBUG;

  // Print the start message
  printf("Volume finding started for %s\n",convert_string(str(square_coordinate)));

  // Register GDAL drivers
  GDALAllRegister();
  // Parse variables from the specified file
  parse_variables(convert_string(file_storage_location+"variables"));
  // Get the start time in microseconds
  unsigned long start_usec = walltime_usec();

  // Read the DEM with borders
  Model<short>* DEM = read_DEM_with_borders(square_coordinate, border);
  // Create a new model for the extent with the same dimensions as the DEM
  Model<bool>* extent = new Model<bool>(DEM->nrows(), DEM->ncols(), MODEL_SET_ZERO);
  extent->set_geodata(DEM->get_geodata());
  // Read the shapefile filter
  string rs(argv[3]);
  read_shp_filter(rs, extent);

  // Initialize the minimum elevation to a large value
  int min_elevation = 100000000;

  // Find the minimum elevation within the extent
  for(int row = 0; row<extent->nrows(); row++)
    for(int col = 0; col<extent->ncols(); col++){
      if(extent->get(row, col))
        min_elevation = MIN(DEM->get(row, col), min_elevation);
    }

  // Initialize arrays for area, volume, and cumulative area at each elevation
  double area_at_elevation[1001] = {0};
  double volume_at_elevation[1001] = {0};
  double cumulative_area_at_elevation[1001] = {0};

  // Calculate the area at each elevation above the minimum elevation
  for(int row = 0; row<extent->nrows(); row++)
    for(int col = 0; col<extent->ncols(); col++)
      if(extent->get(row, col)){
        int elevation_above_pp = MAX(DEM->get(row,col) - min_elevation, 0);
        area_at_elevation[elevation_above_pp+1] += find_area(ArrayCoordinate_init(row, col, DEM->get_origin()));
      }

  // Calculate the cumulative area and volume at each elevation
  for (int ih=1; ih<200;ih++) {
    cumulative_area_at_elevation[ih] = cumulative_area_at_elevation[ih-1] + area_at_elevation[ih];
    volume_at_elevation[ih] = volume_at_elevation[ih-1] + 0.01*cumulative_area_at_elevation[ih]; // area in ha, vol in GL
    printf("%d %d %f   ", ih, min_elevation+ih, volume_at_elevation[ih]);
  }

  // Print the finish message with the runtime
  printf(convert_string("Volume finding finished for "+str(square_coordinate)+". Runtime: %.2f sec\n"), 1.0e-6*(walltime_usec() - start_usec) );
}