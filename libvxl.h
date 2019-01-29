#include <stdint.h>

//! @file libvxl.h
//! Reads and writes vxl maps, using an likewise internal memory format

//! @brief Internal chunk size
//! @note Map is split into square chunks internally to speed up modifications
//! @note Lower values can speed up map access, but can lead to higher memory fragmentation
#define CHUNK_SIZE				16
//! @brief How many blocks the buffer will grow once it is full
#define CHUNK_GROWTH			512

//! @brief The default color to use when a block is solid, but has no color
//!
//! This is the case for e.g. underground blocks which are not visible from the surface
#define DEFAULT_COLOR			0x674028

//! @brief Use Y as top-down axis, where y=0 is water level
#define LIBVXL_COORDS_DEFAULT	0
//! @brief Use the voxlap coordinate system, Z as top-down axis, y=63 is water level
#define LIBVXL_COORDS_VOXLAP	1

//! @brief Used to map coordinates to a key
//!
//! This format is used:
//! @code{.c}
//! 0xYYYXXXZZ
//! @endcode
//! This leaves 12 bits for X and Y and 8 bits for Z
#define pos_key(x,y,z)			(((y)<<20) | ((x)<<8) | (z))
#define key_discardz(key)		((key)&0xFFFFFF00)
#define key_getx(key)			(((key)>>8)&0xFFF)
#define key_gety(key)			(((key)>>20)&0xFFF)
#define key_getz(key)			((key)&0xFF)

struct libvxl_span {
	uint8_t length;
	uint8_t color_start;
	uint8_t color_end;
	uint8_t air_start;
};

struct libvxl_block {
	int32_t position;
	int32_t color;
};

struct libvxl_chunk {
	struct libvxl_block* blocks;
	int length, index;
};

struct libvxl_map {
	int width, height, depth;
	struct libvxl_chunk* chunks;
	struct libvxl_chunk queue;
	uint8_t* geometry;
	int streamed;
};

struct libvxl_stream {
	struct libvxl_map* map;
	int* chunk_offsets;
	int chunk_size;
	void* buffer;
	int buffer_offset;
	int pos;
};

//! @brief Load a map from memory or create an empty one
//!
//! Example:
//! @code{.c}
//! struct libvxl_map m;
//! libvxl_create(&m,512,512,64,ptr);
//! @endcode
//! @param map Pointer to a struct of type libvxl_map that stores information about the loaded map
//! @param w Width of map (x-coord)
//! @param h Height of map (y-coord)
//! @param d Depth of map (z-coord)
//! @param data Pointer to valid map data, left unmodified also not freed
//! @note Pass **NULL** as map data to create a new empty map, just water level will be filled with DEFAULT_COLOR
//! @returns 1 on success
int libvxl_create(struct libvxl_map* map, int w, int h, int d, const void* data, int size);

//! @brief Write a map to disk, uses the libvxl_stream API internally
//! @param map Map to be written
//! @param name Filename of output file
//! @returns total bytes written to disk
int libvxl_writefile(struct libvxl_map* map, char* name);

//! @brief Compress the map back to vxl format and save it in *out*, the total byte size will be written to *size*
//! @param map Map to compress
//! @param out pointer to memory where the vxl will be stored
//! @param size pointer to an int, total byte size
void libvxl_write(struct libvxl_map* map, void* out, int* size);

//! @brief Tells if a block is solid at location [x,y,z]
//! @param map Map to use
//! @param x x-coordinate of block
//! @param y y-coordinate of block
//! @param z z-coordinate of block
//! @returns solid=1, air=0
//! @note Blocks out of map bounds are always non-solid
int libvxl_map_issolid(struct libvxl_map* map, int x, int y, int z);

//! @brief Tells if a block is visible on the surface, meaning it is exposed to air
//! @param map Map to use
//! @param x x-coordinate of block
//! @param y y-coordinate of block
//! @param z z-coordinate of block
//! @returns on surface=1
int libvxl_map_onsurface(struct libvxl_map* map, int x, int y, int z);

//! @brief Read block color
//! @param map Map to use
//! @param x x-coordinate of block
//! @param y y-coordinate of block
//! @param z z-coordinate of block
//! @returns color of block at location [x,y,z] in format *0xAARRGGBB*, on error *0*
int libvxl_map_get(struct libvxl_map* map, int x, int y, int z);

//! @brief Read color of topmost block (as seen from above at Z=0)
//!
//! See libvxl_map_get() for to be expected color format
//!
//! Example:
//! @code{.c}
//! int[2] result;
//! libvxl_map_gettop(&m,256,256,&result);
//! int color = result[0];
//! int height = result[1];
//! @endcode
//! @param map Map to use
//! @param x x-coordinate of block column
//! @param y y-coordinate of block column
//! @param result pointer to *int[2]*, is filled with color at *index 0* and height at *index 1*
//! @note *result* is left unmodified if [x,y,z] is out of map bounds
//! @returns *nothing, see result param*
void libvxl_map_gettop(struct libvxl_map* map, int x, int y, int* result);

//! @brief Set block at location [x,y,z] to a new color
//!
//! See libvxl_map_get() for expected color format, alpha component can be discarded
//!
//! @param map Map to use
//! @param x x-coordinate of block
//! @param y y-coordinate of block
//! @param z z-coordinate of block
//! @param color replacement color
//! @note nothing is changed if [x,y,z] is out of map bounds
void libvxl_map_set(struct libvxl_map* map, int x, int y, int z, int color);

//! @brief Set location [x,y,z] to air, will destroy any block at this position
//! @param map Map to use
//! @param x x-coordinate of block
//! @param y y-coordinate of block
//! @param z z-coordinate of block
void libvxl_map_setair(struct libvxl_map* map, int x, int y, int z);

//! @brief Free a map from memory
//! @param map Map to free
void libvxl_free(struct libvxl_map* map);

//! @brief Tries to guess the size of a map
//! @note This won't always give accurate results for a map's height
//! @note It is assumed the map is square.
//! @param size Pointer to int where edge length of the square will be stored
//! @param depth Pointer to int where map height will be stored
//! @param data Pointer to valid map data, left unmodified also not freed
//! @param len compressed map size in bytes
int libvxl_size(int* size, int* depth, const void* data, int len);

//! @brief Start streaming a map
//! @param stream Pointer to a struct of type libvxl_stream
//! @param map Map to stream
//! @param chunk_size size in bytes each call to libvxl_stream_read() will encode at most
void libvxl_stream(struct libvxl_stream* stream, struct libvxl_map* map, int chunk_size);

//! @brief Free a stream from memory
//! @param map stream to free
void libvxl_stream_free(struct libvxl_stream* stream);

//! @brief Read bytes from the stream, this is at most stream->chunk_size bytes
//! @note The actual byte count encoded is returned, this can be less but never more than chunk_size
//! @note *0* is returned when the stream reached its end
//! @param stream stream to use
//! @param out pointer to buffer where encoded bytes are stored
//! @returns total byte count that was encoded
int libvxl_stream_read(struct libvxl_stream* stream, void* out);
