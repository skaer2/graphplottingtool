attribute vec2 coord3d;
attribute float coordz;
varying vec4 graph_coord;
uniform mat4 texture_transform;
uniform mat4 vertex_transform;

void main(void) {
    graph_coord = texture_transform * vec4(coord3d, 0, 1);

    gl_Position = vertex_transform * vec4(coord3d, coordz, 1);
}
