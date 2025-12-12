
#include "knn_dtw_ops.h"

float euclid_dist(const sigpoints &p1, const sigpoints &p2) {
    float dx = (float)p1.xpos - (float)p2.xpos;
    float dy = (float)p1.ypos - (float)p2.ypos;
    float dz = (float)p1.zpos - (float)p2.zpos;
    
    return sqrt(dx*dx + dy*dy + dz*dz);
}

//Dynamic Time Warping
float dtw_distance(const vector<sigpoints> &vecA, const vector<sigpoints> &vecB) {
    int n = vecA.size();
    int m = vecB.size();
    
    if(n == 0 || m == 0) {
        Serial.println("dtw::: Empty vector provided");
        return INF;
    }
    
    vector<float> prev_row(m + 1, 0);
    vector<float> curr_row(m + 1, 0);
    
    for(int j = 0; j <= m; j++) {
        prev_row[j] = INF;
    }
    curr_row[0] = INF;
    prev_row[0] = 0;
    
    for(int i = 1; i <= n; i++) {
        for(int j = 1; j <= m; j++) {
            float cost = euclid_dist(vecA[i-1], vecB[j-1]);
            float min_prev = min({prev_row[j], curr_row[j-1], prev_row[j-1]});
            curr_row[j] = cost + min_prev;
        }
        prev_row = curr_row;
    }
    
    float distance = prev_row[m];
    float normalized_distance = distance / (n + m);
    
    return normalized_distance;
}

void extract_features(const vector<float> &dtw_distances, float features[]) {
    if(dtw_distances.empty()) {
        for(int i = 0; i < KNN_FEATURES; i++) {
            features[i] = 0.0;
        }
        return;
    }

    vector<float> sorted = dtw_distances;
    sort(sorted.begin(), sorted.end());
    
    int n = sorted.size();
    
    features[0] = sorted[0];
    features[1] = sorted[n - 1];

    float sum = 0;
    for(float d : sorted) {
        sum += d;
    }
    if(n != 0)
    {
        features[2] = sum / n;
    }
    else {
        features[2] = INF;
    }

    float variance = 0;
    for(float d : sorted) {
        float diff = d - features[2];
        variance += diff * diff;
    }

    if(n != 0) {
        features[3] = sqrt(variance / n);
    }
    else 
    {
        features[3] = INF;
    }

    if(n % 2 == 0) {
        features[4] = (sorted[n/2 - 1] + sorted[n/2]) / 2.0;
    } else {
        features[4] = sorted[n/2];
    }
}

void knn_init() {
    Serial.println("knn_init::: Classifier initialized");
    Serial.print("knn_init::: Features: ");
    Serial.println(KNN_FEATURES);
    Serial.print("knn_init::: K value: ");
    Serial.println(KNN_K_VALUE);
    Serial.print("knn_init::: Confidence threshold: ");
    Serial.println(CONFIDENCE_THRESHOLD);
}

bool knn_classify(float* confidence, const vector<float> &new_dtw, int num_pass, int num_fail) {
    Serial.println("\nknn::: Classification");
    Serial.print("knn::: Pass Class Training patterns: ");
    Serial.println(num_pass);
    Serial.print("knn::: Fail Class Training patterns: ");
    Serial.println(num_fail);
    Serial.print("knn::: New DTW values: ");
    Serial.println(new_dtw.size());
    
    //Initial Auto Pass
    if(new_dtw.size() < 4) {
        Serial.println("knn::: First 4 Signatures");
        Serial.println("knn::: AUTO-ACCEPT");
        return true;
    }
        
    //Classification
    float new_features[KNN_FEATURES];
    extract_features(new_dtw, new_features);
    
    int k = min(KNN_K_VALUE, signatureKNN.getCount());
    int classification = signatureKNN.classify(new_features, k);
    *confidence = signatureKNN.confidence();

    Serial.print("knn::: k = ");
    Serial.println(k);

    Serial.print("knn::: Classification: ");
    Serial.println(classification == 1 ? "Genuine" : "Forgery");
    
    Serial.print("knn::: Confidence: ");
    Serial.print((*confidence) * 100);
    Serial.println("%");
    
    bool accepted;
    if(classification == 1)
    {
        accepted = true;
    }
    else if(classification == 0)
    {
        accepted = false;
    }
    Serial.println(accepted ? "knn::: Pass" : "knn::: Fail");
    Serial.println("knn::: Completed\n");
    
    return accepted;
}
