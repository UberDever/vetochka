#include <stddef.h>
#include <stdint.h>

typedef int8_t i8;
typedef size_t uint;
typedef uint Node;

Node node_new_tree(uint lhs, uint rhs);
Node node_new_app(uint lhs, uint rhs);
Node node_new_data(uint data);
Node node_new_invalid();

uint node_tag(Node node);
uint node_lhs(Node node);
uint node_rhs(Node node);
uint node_data(Node node);

uint node_tag_tree();
uint node_tag_app();
uint node_tag_data();
