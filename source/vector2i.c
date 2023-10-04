#include "vector2i.h"

struct Vector2i sum(struct Vector2i v0, struct Vector2i v1){
	struct Vector2i ret = {{v0.x + v1.x}, {v0.y + v1.y}};
	return ret;
}

