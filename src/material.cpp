#include "material.h"
#include "texture.h"
#include "application.h"


StandardMaterial::StandardMaterial()
{
	color = vec4(1.f, 1.f, 1.f, 1.f);
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/flat.fs");
}

StandardMaterial::StandardMaterial(const char* texturename)
{
	color = vec4(1.f, 1.f, 1.f, 1.f);
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs"); //assign texture shader to the material
	texture = Texture::Get(texturename); //get the texture
}

StandardMaterial::~StandardMaterial()
{

}

void StandardMaterial::setUniforms(Camera* camera, Matrix44 model)
{
	//upload node uniforms
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_camera_position", camera->eye);
	shader->setUniform("u_model", model);
	shader->setUniform("u_time", Application::instance->time);

	shader->setUniform("u_color", color);

	if (texture)
		shader->setUniform("u_texture", texture);
}

void StandardMaterial::render(Mesh* mesh, Matrix44 model, Camera* camera)
{
	//set flags
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	if (mesh && shader)
	{
		//enable shader
		shader->enable();

		//upload uniforms
		setUniforms(camera, model);

		//do the draw call
		mesh->render(GL_TRIANGLES);

		//disable shader
		shader->disable();
	}
}

void StandardMaterial::renderInMenu()
{
	ImGui::ColorEdit3("Color", (float*)&color); // Edit 3 floats representing a color
}

void StandardMaterial::update(double s_e) {

}

WireframeMaterial::WireframeMaterial()
{
	color = vec4(1.f, 1.f, 1.f, 1.f);
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/flat.fs");
}

WireframeMaterial::~WireframeMaterial()
{

}

void WireframeMaterial::render(Mesh* mesh, Matrix44 model, Camera * camera)
{
	if (shader && mesh)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		//enable shader
		shader->enable();

		//upload material specific uniforms
		setUniforms(camera, model);

		//do the draw call
		mesh->render(GL_TRIANGLES);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}
/****************************************************************************************/
//skybox material methods
/***************************************************************************************/

SkyboxMaterial::SkyboxMaterial() {

}

SkyboxMaterial::SkyboxMaterial(const char* texturename, bool hdr) {

	texture = new Texture();
	//creating the cubemap (hdre or not)
	if (hdr == false) {
		texture->cubemapFromImages(texturename);
	}
	else {
		selected_texture = 0; //default texture
		//iterate over the array where the texture names are, and we create all the HDRE cubemaps and we store it in
		//the hdre_textures vector
		for (int i = 0; i < 3; i++) {
			hdre_textures.push_back(create_HDRE_texture(texture_options[i]));
		}
		//select the firts texture
		texture = hdre_textures[selected_texture];
		//set as texture name the name of the first texture (hdre) in order to compare it later
		texture_name = texture_options[selected_texture];
	}

	hdre = hdr; //set if we are using hdre textures or not
	
	//getting the skybox shader
	shader = Shader::Get("data/shaders/skybox.vs", "data/shaders/skybox.fs");
}

SkyboxMaterial::~SkyboxMaterial() {

}

void SkyboxMaterial::setUniforms(Camera* camera, Matrix44 model) {

	model.translate(camera->eye.x, camera->eye.y, camera->eye.z); //translating cube model to the camera position

	//uploading uniforms
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_model", model);
	shader->setUniform("u_skybox", texture);

	shader->setUniform("u_hdre", hdre);//is hdre or not

}

void SkyboxMaterial::render(Mesh* mesh, Matrix44 model, Camera * camera) {
	//set flags
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	if (mesh && shader)
	{
		//enable shader
		shader->enable();

		//upload uniforms
		setUniforms(camera, model);

		//do the draw call
		mesh->render(GL_TRIANGLES);

		//disable shader
		shader->disable();
	}
}

void SkyboxMaterial::renderInMenu() {
	if (hdre) {
		//combo in order to select the current hdre texture
		ImGui::Combo("HDRE texture", &selected_texture, texture_options, IM_ARRAYSIZE(texture_options));

		//select the current texture and current texture name
		texture = hdre_textures[selected_texture];
		texture_name = texture_options[selected_texture];
	}
}

Texture* SkyboxMaterial::create_HDRE_texture(const char* hdre_name) {
	HDRE* hdre = HDRE::Get(hdre_name); //create hdre
	unsigned int LEVEL = 0; //level
	Texture* tex = new Texture(); 
	tex->cubemapFromHDRE(hdre, LEVEL); //create cubemap
	return tex;
}

//update
void SkyboxMaterial::update(double s_e) {

}

/***************************************************************************************/

//PBR material methods
/***************************************************************************************/
PBRmaterial::PBRmaterial() {


}

PBRmaterial::PBRmaterial(const char* texture_names[], bool _opacity, bool _emissive) {
	color = vec4(1.f, 1.f, 1.f, 1.f); //default color
	shader = Shader::Get("data/shaders/pbr.vs", "data/shaders/pbr.fs"); //getting the pbr shaders

	for (int i = 0; i < 8; i++) {
		Texture* tex = Texture::Get(texture_names[i]);
		textures.push_back(tex);
	}

	opacity = _opacity; emissive = _emissive;
	gamma = true; direct = true; ambient = true;

	//Init extra properties
	if (opacity && emissive) {
		extra_properties = BOTH;
		ishelmet = true;
	}
	else if (opacity && !emissive) {
		extra_properties = OPACITY;
		ishelmet = false;
	}
	else if (!opacity && emissive) {
		extra_properties = EMISSIVE;
		ishelmet = true;
	}

	change_hdre();//create hdre levels vector
}

PBRmaterial::~PBRmaterial()
{
}

void PBRmaterial::setUniforms(Camera* camera, Matrix44 model) {
	//upload node uniforms
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_camera_position", camera->eye);
	shader->setUniform("u_model", model);
	shader->setUniform("u_time", Application::instance->time);

	shader->setUniform("u_color", color);

	const char* uniform_tex_names[] = { {"u_albedo"},{"u_roughness"},{"u_metalness"},{"u_brdfLUT"},
										{"u_normal_map"},{"u_ao_map"},{"u_opacity_map"},{"u_emission_map"} };

	//enter textures in the shader
	for (int i = 0; i < 8; i++) {
		if(textures[i])
			shader->setUniform(uniform_tex_names[i], textures[i], i);
	}

	shader->setUniform("u_light_pos", light.pos); //direct light position
	shader->setUniform("u_light_color", light.diffuse); //direct light color

	//enter hdre levels
	shader->setUniform("u_texture_prem", hdr_l[0], 8);
	shader->setUniform("u_texture_prem_0", hdr_l[1], 9);
	shader->setUniform("u_texture_prem_1", hdr_l[2], 10);
	shader->setUniform("u_texture_prem_2", hdr_l[3], 11);
	shader->setUniform("u_texture_prem_3", hdr_l[4], 12);
	shader->setUniform("u_texture_prem_4", hdr_l[5], 13);

	//enter booleans
	shader->setUniform("u_gamma_correction", gamma);

	shader->setUniform("u_emission", emissive);
	shader->setUniform("u_opacity", opacity);
	shader->setUniform("u_direct_light", direct);
	shader->setUniform("u_IBL", ambient);
	shader->setUniform("u_helmet", ishelmet);

}

void PBRmaterial::render(Mesh* mesh, Matrix44 model, Camera * camera) {
	//set flags
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	if (opacity) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	if (mesh && shader)
	{
		//enable shader
		shader->enable();

		//upload uniforms
		setUniforms(camera, model);

		//do the draw call
		mesh->render(GL_TRIANGLES);

		//disable shader
		shader->disable();
	}

	if (opacity) {
		glDisable(GL_BLEND);
	}
	
}

void PBRmaterial::renderInMenu() {

	ImGui::ColorEdit3("Color", (float*)&color); // Edit 3 floats representing a color
	ImGui::DragFloat3("Light Position", (float*)&light.pos); //light position edit
	ImGui::ColorEdit3("Light Color", (float*)&light.diffuse);//direct light color edit
	ImGui::Checkbox("Direct Light", &direct); //activate/deactivate direct light
	ImGui::Checkbox("IBL", &ambient); //activate/deactivate IBL

	//activate/deactivate extra properties
	switch (extra_properties)
	{
	case OPACITY:
		ImGui::Checkbox("Opacity", &opacity);
		break;
	case EMISSIVE:
		ImGui::Checkbox("Emissive properties", &emissive);
		break;
	case BOTH:
		ImGui::Checkbox("Opacity", &opacity);
		ImGui::Checkbox("Emissive properties", &emissive);
		break;
	default:
		break;
	}

	ImGui::Checkbox("Gamma correction", &gamma); //activate/deactivate gamma correction

}

void PBRmaterial::change_hdre() {
	hdr_l.clear(); //clear the hdre levels vector
	//get the current HDRE 
	//it will always be the same hdre of the skybox
	hdre_name = Application::instance->skybox_node->skybox_material->texture_name; 
	HDRE* hdre = HDRE::Get(hdre_name); //create hdre
	//create all levels of hdre (0 + 5)
	for (int i = 0; i <= 5; i++) {
		unsigned int LEVEL = i;
		Texture* tex = new Texture();
		tex->cubemapFromHDRE(hdre, (unsigned int)i);
		hdr_l.push_back(tex);
	}
}

void PBRmaterial::update(double s_e) {
	//reload hdre
	if (hdre_name != Application::instance->skybox_node->skybox_material->texture_name) {
		change_hdre();
	}
}

/***************************************************************************************/