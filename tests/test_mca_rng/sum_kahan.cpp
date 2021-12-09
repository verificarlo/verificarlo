#include <stdlib.h>
#include <stdio.h>
#include <vector>


float KahanSum(const vector<float> &values)
{
    float sum = 0.0f;
    float c = 0.0f;
    
    for (float v : values)
    {
        float y = v - c;
        float t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }
    
    return sum;
}