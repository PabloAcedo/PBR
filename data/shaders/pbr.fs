#define PI 3.14159265359
#define RECIPROCAL_PI 0.3183098861837697

#define GAMMA 2.2
#define INV_GAMMA 0.45

//structs
struct sPBRMaterial
{
	vec3 albedo; //albedo color RGB
	vec3 F0; //Fresnel value RGB
	vec3 Cdiffuse; //diffuse value
	float metalness; //metalness
	float roughness; //roughness
	vec3 ao; //baked ambient occlusion
	float opacity;//opacity
	vec3 emission; //emissive
	
};

struct sVectors{
	vec3 N;
	vec3 L;
	vec3 V;
	vec3 R;
	vec3 H;
};

//varyings
varying vec3 v_position;
varying vec3 v_world_position;
varying vec3 v_normal;
varying vec2 v_uv;
varying vec4 v_color;

//uniforms
uniform vec4 u_color;
uniform float u_time;
uniform vec3 u_light_pos;
uniform vec3 u_camera_position;
uniform vec3 u_light_color;

uniform bool u_gamma_correction;
uniform bool u_emission;
uniform bool u_opacity;
uniform bool u_helmet;
uniform bool u_direct_light;
uniform bool u_IBL;

	//textures
uniform sampler2D u_albedo;
uniform sampler2D u_metalness;
uniform sampler2D u_roughness;
uniform sampler2D u_brdfLUT;
uniform sampler2D u_normal_map;
uniform sampler2D u_ao_map;
uniform sampler2D u_opacity_map;
uniform sampler2D u_emission_map;

	// Levels of the HDR Environment to simulate roughness material(IBL)
uniform samplerCube u_texture_prem;
uniform samplerCube u_texture_prem_0;
uniform samplerCube u_texture_prem_1;
uniform samplerCube u_texture_prem_2;
uniform samplerCube u_texture_prem_3;
uniform samplerCube u_texture_prem_4;


//functions
vec3 getReflectionColor(vec3 r, float roughness)
{
	float lod = roughness * 5.0;

	vec4 color;

	if(lod < 1.0) color = mix( textureCube(u_texture_prem, r), textureCube(u_texture_prem_0, r), lod );
	else if(lod < 2.0) color = mix( textureCube(u_texture_prem_0, r), textureCube(u_texture_prem_1, r), lod - 1.0 );
	else if(lod < 3.0) color = mix( textureCube(u_texture_prem_1, r), textureCube(u_texture_prem_2, r), lod - 2.0 );
	else if(lod < 4.0) color = mix( textureCube(u_texture_prem_2, r), textureCube(u_texture_prem_3, r), lod - 3.0 );
	else if(lod < 5.0) color = mix( textureCube(u_texture_prem_3, r), textureCube(u_texture_prem_4, r), lod - 4.0 );
	else color = textureCube(u_texture_prem_4, r);

	// Gamma correction
	//color = pow(color, vec4(1.0/GAMMA));

	return color.xyz;
}

//Javi Agenjo Snipet for Bump Mapping
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv){
	// get edge vectors of the pixel triangle
	vec3 dp1 = dFdx( p );
	vec3 dp2 = dFdy( p );
	vec2 duv1 = dFdx( uv );
	vec2 duv2 = dFdy( uv );

	// solve the linear system
	vec3 dp2perp = cross( dp2, N );
	vec3 dp1perp = cross( N, dp1 );
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	// construct a scale-invariant frame
	float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
	return mat3( T * invmax, B * invmax, N );
}

vec3 perturbNormal( vec3 N, vec3 V, vec2 texcoord, vec3 normal_pixel ){
	#ifdef USE_POINTS
	return N;
	#endif

	// assume N, the interpolated vertex normal and
	// V, the view vector (vertex to eye)
	//vec3 normal_pixel = texture2D(normalmap, texcoord ).xyz;
	normal_pixel = normal_pixel * 255./127. - 128./127.;
	mat3 TBN = cotangent_frame(N, V, texcoord);
	return normalize(TBN * normal_pixel);
}


vec3 toneMap(vec3 color)
{
    return color / (color + vec3(1.0));
}

// degamma
vec3 gamma_to_linear(vec3 color)
{
	return pow(color, vec3(GAMMA));
}

// gamma
vec3 linear_to_gamma(vec3 color)
{
	return pow(color, vec3(INV_GAMMA));
}


//init all the material properties
sPBRMaterial init_material(vec2 uv){
	sPBRMaterial material;

	material.albedo = texture2D( u_albedo, uv ).xyz; 
	//degamma
	if(u_gamma_correction){
		material.albedo = gamma_to_linear(material.albedo);
	}
	
	if(u_helmet){
		material.roughness = max(0.01, texture2D( u_roughness, uv ).y); //for helmet
		material.metalness = min(texture2D( u_roughness, uv ).z,0.99);//for helmet
	}else{
		material.roughness = max(0.01, texture2D( u_roughness, uv ).x); 
		material.metalness = min(texture2D( u_metalness, uv ).x,0.99);
	}

	material.F0 = material.albedo*vec3(material.metalness) + (vec3(1.0) - vec3(material.metalness))*vec3(0.04);//linear interpolation
	material.Cdiffuse = material.albedo*(vec3(1.0) - vec3(material.metalness));//linear interpolation

	material.ao = texture2D(u_ao_map, uv).xyz; //ambient occlusion map
	material.opacity = texture2D(u_opacity_map,uv).x; //opacity map
	material.emission = texture2D(u_emission_map,uv).xyz;//emission map
	return material;
}

//compute all the light equation vectors
sVectors init_vectors(vec2 uv){
	sVectors vectors;

	vectors.N = normalize(v_normal);
	vectors.L = normalize(u_light_pos - v_world_position);	
	vectors.V = normalize(u_camera_position - v_world_position);
	vectors.R = normalize(reflect(-vectors.L,vectors.N));
	vectors.H = normalize(vectors.V + vectors.L);
	
	//Normal mapping (using perturbed normal)
	vec3 normal_pixel = texture2D(u_normal_map, uv).xyz;
	vectors.N = normalize(perturbNormal(vectors.N, vectors.V, uv, normal_pixel));

	return vectors;
}

//compute D (NDF)
float compute_D(float roughness, float NdotH){
	float alpha_2 = pow(roughness,2.0);
	float den = pow(pow(NdotH,2.0)*(alpha_2-1.0)+1,2.0);
	float D = alpha_2/(PI*den);
	return D;
}

//compute the fresnel value
vec3 compute_fresnel(vec3 F0, float LdotH){

	vec3 F = F0 + (vec3(1.0)- F0)*pow(1.0-LdotH,5.0);

	return F;
}



//compute G (Geometric attenuation factor for for self-shadowing microfacets)
float compute_G(float roughness, float NdotL, float NdotV){
	float k = pow(roughness+1.0,2.0)/8.0;
	float G1 = NdotL/(NdotL*(1-k)+k);
	float G2 = NdotV/(NdotV*(1-k)+k);

	return G1*G2;
}

//compute direct brdf
vec3 compute_Dir_BRDF(sPBRMaterial material, sVectors vectors){
	
	//compute vector operations
	float low_clamp_factor = 0.001;
	float NdotL = max(low_clamp_factor,dot(vectors.N,vectors.L));
	float NdotV = max(low_clamp_factor,dot(vectors.N,vectors.V));
	float NdotH = max(low_clamp_factor,dot(vectors.N,vectors.H));
	float LdotH = max(low_clamp_factor,dot(vectors.L, vectors.H));

	//diffuse f
	vec3 f_diffuse = material.Cdiffuse/PI;

	//compute F,G,D
	vec3 F = compute_fresnel(material.F0, LdotH);
	float G = compute_G(material.roughness, NdotL, NdotV);
	float D = compute_D(material.roughness, NdotH);
	
	//compute f specular
	vec3 f_specular = (F*G*D)/(4.0*NdotL*NdotV);
	//compute final f (BRDF)
	vec3 f = (f_diffuse+f_specular)*NdotL*u_light_color;
	
	return f;
}

//indirect lighting
vec3 IBL(sPBRMaterial material, sVectors vectors){

	//diffuse component
	vec3 diffuseSample = getReflectionColor(vectors.N, 1.0);
	vec3 diffuseIBL = material.Cdiffuse*diffuseSample;

	//specular component
	vec3 R = normalize(reflect(-vectors.V,vectors.N));//reflection from the V vector
	vec3 specularSample = getReflectionColor(R, material.roughness);

	float NdotV = max(0.01,dot(vectors.N,vectors.V));
	vec4 BRDF = texture2D(u_brdfLUT, vec2(NdotV,material.roughness)); //reading the precomputed BRDF
	vec3 specularBRDF = material.F0 * BRDF.x + BRDF.y; //specular BRDF

	vec3 specularIBL = specularSample * specularBRDF; 
	
	return (specularIBL+diffuseIBL)*material.ao;
}


void main()
{
	vec2 uv = v_uv;

	vec3 final_color = vec3(0.0);
	
	//init material properties and vectors
	sPBRMaterial material = init_material(uv);
	sVectors vectors = init_vectors(uv);

	//direct pbr
	if(u_direct_light){
		final_color += compute_Dir_BRDF(material, vectors);
	}

	//Image Based Lighting
	if(u_IBL){
		final_color += IBL(material, vectors);
	}

	//tone mapping
	final_color = toneMap(final_color);

	//add emissive properties of the material (if applicable)
	if(u_emission){
		final_color += material.emission;//helmet only
	}
	
	//gamma correction
	if(u_gamma_correction){
		final_color = linear_to_gamma(u_color.xyz*final_color); //apply gamma correction
	}

	vec4 color;

	//color alpha changes(if applicable)
	if(u_opacity){
		color = vec4(final_color,material.opacity);//lantern only
	}else{
		color = vec4(final_color,1.0);
	}
	
	gl_FragColor = color;

}
