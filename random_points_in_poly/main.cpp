#include "poly2tri.h"
#include "common/random_utils.h"

#include "shapefil.h"

#include <iostream>


int main()
{
    using namespace p2t;
    p2t::random_points_in_triangle(110., 35., 120., 35., 115., 40., 10000, "out.csv");
    return 0;
}
