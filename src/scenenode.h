#ifndef SCENENODE_H
#define SCENENODE_H

#include "framework.h"
#include "shader.h"
#include "mesh.h"
#include "camera.h"
#include "material.h"

class SceneNode {
public:

	static unsigned int lastNameId;

	SceneNode();
	SceneNode(const char* name);
	~SceneNode();

	bool render_node;

	Material * material = NULL;
	std::string name;

	Mesh* mesh = NULL;
	Matrix44 model;

	virtual void render(Camera* camera);
	virtual void renderWireframe(Camera* camera);
	virtual void renderInMenu();
	virtual void update(double seconds_elapsed);
};

class SkyboxNode : public SceneNode {
public:
	SkyboxMaterial* skybox_material;
	SkyboxNode();
	SkyboxNode(const char* name);
	SkyboxNode(const char* name, const char* skytextname, bool hd);
	~SkyboxNode();

	void render(Camera* camera);
	void renderWireframe(Camera* camera);
	void renderInMenu();
	void update(double seconds_elapsed);
};
#endif