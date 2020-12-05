#include "scenenode.h"
#include "application.h"
#include "texture.h"
#include "utils.h"

unsigned int SceneNode::lastNameId = 0;

SceneNode::SceneNode()
{
	this->name = std::string("Node" + std::to_string(lastNameId++));
	render_node = true;
}


SceneNode::SceneNode(const char * name)
{
	this->name = name;
}

SceneNode::~SceneNode()
{

}

void SceneNode::render(Camera* camera)
{
	if (material && render_node)
		material->render(mesh, model, camera);
}

void SceneNode::renderWireframe(Camera* camera)
{
	WireframeMaterial mat = WireframeMaterial();
	mat.render(mesh, model, camera);
}

void SceneNode::renderInMenu()
{
	ImGui::Checkbox("Render node", &render_node);

	//Model edit
	if (ImGui::TreeNode("Model")) 
	{
		float matrixTranslation[3], matrixRotation[3], matrixScale[3];
		ImGuizmo::DecomposeMatrixToComponents(model.m, matrixTranslation, matrixRotation, matrixScale);
		ImGui::DragFloat3("Position", matrixTranslation, 0.1f);
		ImGui::DragFloat3("Rotation", matrixRotation, 0.1f);
		ImGui::DragFloat3("Scale", matrixScale, 0.1f);
		ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, model.m);
		
		ImGui::TreePop();
	}

	//Material
	if (material && ImGui::TreeNode("Material"))
	{
		material->renderInMenu();
		ImGui::TreePop();
	}
}

void SceneNode::update(double seconds_elapsed) {
	if (material) {
		material->update(seconds_elapsed);
	}
}

//skybox node class methods
/***************************************************************************************/

SkyboxNode::SkyboxNode() : SceneNode() {

	mesh->createCube();
	

}

SkyboxNode::SkyboxNode(const char* name) {

	mesh->createCube();
	this->name = name;
	render_node = true;
}

SkyboxNode::SkyboxNode(const char* name, const char* skytextname, bool hd) {
	this->name = name;

	//creating the cube
	mesh = new Mesh();
	mesh->createCube();

	//scaling the cube 
	for (int i = 0; i < mesh->vertices.size(); i++)
		mesh->vertices[i] *= 3000;

	//creating the material
	skybox_material = new SkyboxMaterial(skytextname, hd);

}

SkyboxNode::~SkyboxNode() {

}

void SkyboxNode::render(Camera* camera) {
	if (skybox_material && render_node) {
		//disable the zbuffer
		glDisable(GL_DEPTH_TEST); 
		skybox_material->render(mesh, model, camera);
		glEnable(GL_DEPTH_TEST);
	}
		
}

void SkyboxNode::renderWireframe(Camera* camera) {

}

void SkyboxNode::renderInMenu() {
	ImGui::Checkbox("Render node", &render_node);
	//Material
	if (skybox_material && ImGui::TreeNode("Material"))
	{
		skybox_material->renderInMenu();
		ImGui::TreePop();
	}
}

void SkyboxNode::update(double seconds_elapsed) {
	if (skybox_material) {
		skybox_material->update(seconds_elapsed);
	}
}