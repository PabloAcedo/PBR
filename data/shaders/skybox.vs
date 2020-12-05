
attribute vec3 a_vertex;

varying vec3 v_texcoords;

uniform mat4 u_viewprojection;
uniform mat4 u_model;

void main()
{	
    v_texcoords = a_vertex; //the texure coords are the vertex coordinates
    vec4 pos = u_viewprojection * u_model* vec4(a_vertex, 1.0); //computing the position
    gl_Position = pos;
    
}  