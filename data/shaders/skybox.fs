

varying vec3 v_texcoords;

uniform samplerCube u_skybox;
uniform bool u_hdre;

vec3 toneMap(vec3 color)
{
    return color / (color + vec3(1.0));
}

// gamma
vec3 linear_to_gamma(vec3 color)
{
	return pow(color, vec3(0.45));
}

void main()
{   

    vec4 color = textureCube(u_skybox, v_texcoords);
    
    //calculations if the texture is hdre
    if (u_hdre){
	vec3 aux_color = toneMap(color.xyz);
	aux_color = linear_to_gamma(aux_color);
	color = vec4(aux_color,1.0);
    }

    gl_FragColor = color; 
}
