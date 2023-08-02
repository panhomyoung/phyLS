#ifndef EXACT_LUT_HPP
#define EXACT_LUT_HPP

#include <math.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace phyLS {
void exact_lut(vector<string>& tt, int& input);
void exact_lut_enu(vector<string>& tt, int& input);
}  // namespace phyLS

#endif
