#ifndef __DRIVER__
#define __DRIVER__
#include "common.h"
#include <vector>

struct driver_state
{
    // Custom data that is stored per vertex, such as positions or colors.
    // These fields are stored contiguously, interleaved.  For example,
    // X Y Z R G B X Y Z R G B X Y Z R G B ...
    // Each vertex occupies floats_per_vertex entries in the array.
    // There are num_vertices vertices and thus floats_per_vertex*num_vertices
    // floats in the array.
    float * vertex_data = 0;
    int num_vertices = 0;
    int floats_per_vertex = 0;

    // If indexed rendering is being performed, this array stores the vertex
    // indices for the triangles, three ints per triangle.
    // i j k i j k i j k i j k ...
    // There are num_triangles triangles, so the array contains 3*num_triangles
    // entries.
    int * index_data = 0;
    int num_triangles = 0;

    // This is data that is constant over all triangles and fragments.
    // It is accessible from all of the shaders.  The user can store things
    // like transforms here.  The size of this array is not stored
    // since the driver will never need to know its size; you will just need.
    // to supply the pointer when necessary.
    float * uniform_data = 0;

    // Vertex data (such as color) at the vertices of triangles must be
    // interpolated to each pixel (fragment) within the triangle before calling
    // the fragment shader.  Since there are floats_per_vertex floats stored per
    // vertex, there will be floats_per_vertex valid entries in this array,
    // indicating how each float for a vertex should be interpolated.  Valid
    // values are:
    //   interp_type::flat           - each pixel receives the value stored at the first
    //                                 vertex of the triangle.
    //   interp_type::smooth         - Vertex values are interpolated using perspective-
    //                                 correct interpolation.
    //   interp_type::noperspective  - Vertex values are interpolated using image-space
    //                                 barycentric coordinates.
    interp_type interp_rules[MAX_FLOATS_PER_VERTEX] = {};

    // Image dimensions
    int image_width = 0;
    int image_height = 0;

    // Convenience var for length of arrays
    int image_len = 0;

    // Buffer where color data is stored.  The first image_width entries
    // correspond to the bottom row of the image, the next image_width entries
    // correspond to the next row, etc.  The array has image_width*image_height
    // entries.
    pixel * image_color = 0;

    // This array stores the depth of a pixel and is used for z-buffering.  The
    // size and layout is the same as image_color.
    float * image_depth = 0;

    // Pointer to a function, which performs the role of a vertex shader.  It
    // should be called on each vertex and given data stored in vertex_data.
    // This routine also receives the uniform data.
    void (*vertex_shader)(const data_vertex& in, data_geometry& out,
        const float * uniform_data);

    // Pointer to a function, which performs the role of a fragment shader.  It
    // should be called for each pixel (fragment) within each triangle.  The
    // fragment shader should be given interpolated vertex data (interpolated
    // according to interp_rules).  This routine also receives the uniform data.
    void (*fragment_shader)(const data_fragment& in, data_output& out,
        const float * uniform_data);

    driver_state();
    ~driver_state();
};

// Set up the internal state of this class.  This is not done during the
// constructor since the width and height are not known when this class is
// constructed.
void initialize_render(driver_state& state, int width, int height);

// This function will be called to render the data that has been stored in this class.
// Valid values of type are:
//   render_type::triangle - Each group of three vertices corresponds to a triangle.
//   render_type::indexed -  Each group of three indices in index_data corresponds
//                           to a triangle.  These numbers are indices into vertex_data.
//   render_type::fan -      The vertices are to be interpreted as a triangle fan.
//   render_type::strip -    The vertices are to be interpreted as a triangle strip.
void render(driver_state& state, render_type type);

// This function clips a triangle (defined by the three vertices in the "in" array).
// It will be called recursively, once for each clipping face (face=0, 1, ..., 5) to
// clip against each of the clipping faces in turn.  When face=6, clip_triangle should
// simply pass the call on to rasterize_triangle.
void clip_triangle(driver_state& state, const data_geometry* in[3],int face=0);

// Rasterize the triangle defined by the three vertices in the "in" array.  This
// function is responsible for rasterization, interpolation of data to
// fragments, calling the fragment shader, and z-buffering.
void rasterize_triangle(driver_state& state, const data_geometry* in[3]);

/**************************************************************************/
/* Initialization */
/**************************************************************************/

// Sets each pixel in the image_color array to black
void set_render_black(driver_state& state);

// Sets each element of image_depth to FLOAT_MAX
void init_image_depth(driver_state& state);


/**************************************************************************/
/* Render Helpers */
/**************************************************************************/

// The following 4 functions fill data geometry array with pointers to
// vertecies from driver_state's vertex_data array
// The way they do this is specified by their render_type suffix
void fill_data_geo_triangle(const driver_state& state,
    data_geometry * data_geos[3], int & vert_index);

void fill_data_geos_indexed(const driver_state& state,
    data_geometry * data_geos[3], int & vert_index);

void fill_data_geos_fan(const driver_state& state,
    data_geometry * data_geos[3], int & vert_index);

void fill_data_geos_strip(const driver_state& state,
    data_geometry * data_geos[3], int & vert_index, int iteration);

void calc_data_geo_pos(driver_state& state, data_geometry * data_geos[3]);

/**************************************************************************/
/* Rasterize Triangle Helpers */
/**************************************************************************/

void calc_pixel_coords(driver_state& state, const data_geometry& data_geo,
    float & i, float & j);

// Calculates the minimum x and y pixel coordinates for the given triangle
void calc_min_coord(const driver_state& state, float * x, float * y, 
    float & min_x, float & min_y);

// Calculates the maximum x and y pixel coordinates for the given triangle
void calc_max_coord(const driver_state& state, float * x, float * y,
    float & max_x, float& max_y);

bool is_pixel_inside(float * bary_weights);


/**************************************************************************/
/* Fragment Shader */
/**************************************************************************/

// Fills data_fragment's data array with interpolated data then calls the
// state's fragment shader on the interpolated data
pixel get_pixel_color(driver_state& state, data_fragment& frag,
    const data_geometry * data_geos[3], float * screen_bary); 

// Calculates the interpolated data at the specified float of the vertex
float interpolate_fragment_at(unsigned index, 
    const data_geometry * data_geos[3], float * bary);

// Calculates the world space barycentric coordinates from the screen space
// barycentric coordinates 
void convert_from_screen(float * screen_bary, float * world_bary,
    const data_geometry * data_geos[3]);


/**************************************************************************/
/* Z-Buffer */
/**************************************************************************/
void calc_z_coords(const data_geometry * data_geos[3], float * z);

float calc_depth_at(float * z, float * bary);


/**************************************************************************/
/* Clipping */
/**************************************************************************/
// Calls remove_data_geo on each index, then clears the vector.
void clear_data_geos(std::vector<data_geometry *>& tris);

// Deletes the allocated data array for each geometry, then deletes the
// data_geometry array at the specified index
void remove_data_geo(unsigned index, std::vector<data_geometry *>& tris);

// Add a new data_geometry array to the vector using data from the data_geos
// Data is copied.
void add_data_geos(const driver_state& state,
    std::vector<data_geometry *>& tris, const data_geometry * data_geos[3]);

// Add a new data_geometry array to the vector from 3 gl_Positions
void add_data_geos(std::vector<data_geometry *>& tris, const vec4& a, 
    const vec4& b, const vec4& c);

// Copy each float from one data_geometry's data to another
void copy_data_geos_data(const driver_state& state,
    const data_geometry& from, data_geometry& to);

void copy_data_geos_data(const driver_state& state, 
    const data_geometry * from[3], data_geometry * to[3], unsigned a,
    unsigned b, unsigned c);

// Convenience function to check if all vertices are inside
bool all_inside(bool * inside);

// Convenience function to check if all vertices are outside
bool all_outside(bool * inside);

// Create a new triangle with two vertices outside of the plane 
void create_triangle_2_out(std::vector<data_geometry *>& tris,
    unsigned axis, int sign, unsigned in_index, unsigned out0_index,
    unsigned out1_index, const driver_state& state);

// Create two new triangle with two vertices inside of the plane
void create_triangle_2_in(std::vector<data_geometry *>& tris,
    unsigned axis, int sign, unsigned out_index, unsigned in0_index,
    unsigned in1_index, const driver_state& state);

float interpolate_data(float weight, float data0, float data1);

float calc_noperspective_weight(float weight, float a_w, float p_w);

void print_data_geos(const data_geometry * data_geos[3]);

#endif
