# pragma once
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


struct Vector2i sum(struct Vector2i v0, struct Vector2i v1);

