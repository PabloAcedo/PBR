#ifndef MATERIAL_H
#define MATERIAL_H

#include "framework.h"
#include "shader.h"
#include "camera.h"
#include "mesh.h"

//struct that represent a light
struct sLight {
	vec3 pos = vec3(50.0,50.0,0.0); //position
	vec3 ambient; //ambient color
	vec3 diffuse = vec3(1.0,1.0,1.0); //diffuse color
	vec3 specular = vec3(1.0, 1.0, 1.0); //specular color
};

enum eMaterialType : int {STANDARD, SKYBOX, PBR};

class Material {
public:

	eMaterialType type;

	Shader* shader = NULL;
	Texture* texture = NULL;
	vec4 color;

	virtual void setUniforms(Camera* camera, Matrix44 model) = 0;
	virtual void render(Mesh* mesh, Matrix44 model, Camera * camera) = 0;
	virtual void renderInMenu() = 0;
	virtual void update(double seconds_elapsed) = 0;
};

class StandardMaterial : public Material {
public:

	StandardMaterial();
	StandardMaterial(const char* texturename);
	~StandardMaterial();

	void setUniforms(Camera* camera, Matrix44 model);
	void render(Mesh* mesh, Matrix44 model, Camera * camera);
	void renderInMenu();
	void update(double seconds_elapsed);
};

class WireframeMaterial : public StandardMaterial {
public:

	WireframeMaterial();
	~WireframeMaterial();

	void render(Mesh* mesh, Matrix44 model, Camera * camera);
};

//skybox material class
class SkyboxMaterial : public Material {
public:

	bool hdre; //if it is hdre or not
	const char* texture_name; //hdre or skybox texture name

	const char* texture_options[3] = {{"data/environments/studio.hdre"},
									{"data/environments/tv_studio.hdre"},
									{"data/environments/panorama.hdre"}}; //texture names vector
	std::vector<Texture*> hdre_textures; //vector of created hdre
	int selected_texture; //current selected texture

	SkyboxMaterial();
	SkyboxMaterial(const char* texturename, bool hdr);
	~SkyboxMaterial();

	void setUniforms(Camera* camera, Matrix44 model);
	void render(Mesh* mesh, Matrix44 model, Camera * camera);
	void renderInMenu();
	Texture* create_HDRE_texture(const char* hdre_name); //create an hdre
	void update(double seconds_elapsed);
};

//pbr material
enum eMaterialExtras : int {OPACITY, EMISSIVE, BOTH};

class PBRmaterial : public Material {
public:

	sLight light; //direct light

	bool gamma;//activate/deactivate gamma correction
	bool opacity; //activate/deactivate opacity of the material (if it has opacity)
	bool emissive; //activate/deactivate emissive parts in the material (if it has)
	bool direct;//activate/deactivate direct light 
	bool ambient; //activate/deactivate ambient light(IBL)
	bool ishelmet;//helmet stuff (the helmet needs a special lecture of roughnes and metalness in the shader)

	eMaterialExtras extra_properties;//in order to see if the material has opacity, emissive or both features

	std::vector< Texture* > textures; //textures vector (containing all the textures of the material)

	//HDRE
	std::vector< Texture* > hdr_l; //all the hdr levels of a given hdre
	const char* hdre_name; //in order to compare it with the hdre of the skybox

	PBRmaterial();
	PBRmaterial(const char* texture_names[], bool opacity, bool emissive);
	~PBRmaterial();

	void setUniforms(Camera* camera, Matrix44 model);
	void render(Mesh* mesh, Matrix44 model, Camera * camera);
	void renderInMenu();
	void change_hdre(); //create new hdre filtered (all levels) textures
	void update(double seconds_elapsed); //update
};


#endif