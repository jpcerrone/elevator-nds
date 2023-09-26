# pragma once
//#include <math.h>
struct Vector2i {
    union {
        int x;
        int width;
    };
    union {
        int y;
        int height;
    };
};
/*
float distance(Vector2i v0, Vector2i v1){
	float dy = (float)abs(v1.y - v0.y);
	float dx = (float)abs(v1.x - v0.x);
	return sqrtf((float)(pow(dy, 2.0f) + pow(dx,2.0f )));
}
*/
/*
struct Vector2i sum(Vector2i v0, Vector2i v1){
	return {v0.x + v1.x, v0.y + v1.y};
}
*/
