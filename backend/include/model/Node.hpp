#pragma once

namespace model {

/*
 * Node and Road are primarily data containers (POD-like structs).
 * Their public fields are intentionally simple since behavior and 
 * logic (like routing or simulation) live in dedicated engine classes.
 */
struct Node {
    int id;
    double x;
    double y;
};

} // namespace model

