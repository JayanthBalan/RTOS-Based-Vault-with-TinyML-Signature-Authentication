
#ifndef KNN_DTW_OPS_H
#define KNN_DTW_OPS_H

//Libraries
#include "type_dat.h"
#include <cmath>
#include <algorithm>

//Function Prototypes
float dtw_distance(const vector<sigpoints>&, const vector<sigpoints>&);
void knn_init();
bool knn_classify(float*, const vector<float>&, int, int);
void extract_features(const vector<float>&, float[]);

//Classifier Configurations
#define KNN_K_VALUE 3
#define GENUINE_CLASS 1
#define FORGERY_CLASS 0
#define CONFIDENCE_THRESHOLD 0.5

//Global
#define INF 999999.0

#endif
