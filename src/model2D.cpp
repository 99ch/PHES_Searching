#include "model2D.h"

// Specialization of the flows_to function for Model<char>
template<> bool Model<char>::flows_to(ArrayCoordinate c1, ArrayCoordinate c2) {
	// Check if the direction from c1 leads to c2
	return ( ( c1.row + directions[this->get(c1.row,c1.col)].row == c2.row ) &&
		 ( c1.col + directions[this->get(c1.row,c1.col)].col == c2.col ) );
}
