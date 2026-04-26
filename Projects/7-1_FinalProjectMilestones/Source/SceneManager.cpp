///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	// initialize texture counter so loaded textures are tracked correctly
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	// gravel path surface
	CreateGLTexture("../../Utilities/textures/gravel.jpg", "gravel");

	// hedge foliage
	CreateGLTexture("../../Utilities/textures/hedge.jpg", "hedge");

	// activate all loaded textures
	BindGLTextures();

	/*** OBJECT MATERIALS ***/

	// hedge foliage material: matte green with low specular
	OBJECT_MATERIAL hedgeMaterial;
	hedgeMaterial.ambientColor = glm::vec3(0.1f, 0.2f, 0.1f);
	hedgeMaterial.ambientStrength = 0.4f;
	hedgeMaterial.diffuseColor = glm::vec3(0.3f, 0.6f, 0.3f);
	hedgeMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	hedgeMaterial.shininess = 8.0f;
	hedgeMaterial.tag = "hedge_material";
	m_objectMaterials.push_back(hedgeMaterial);

	// gravel path material: dull and granular, almost no specular highlight
	OBJECT_MATERIAL gravelMaterial;
	gravelMaterial.ambientColor = glm::vec3(0.3f, 0.25f, 0.2f);
	gravelMaterial.ambientStrength = 0.3f;
	gravelMaterial.diffuseColor = glm::vec3(0.5f, 0.45f, 0.35f);
	gravelMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	gravelMaterial.shininess = 2.0f;
	gravelMaterial.tag = "gravel_material";
	m_objectMaterials.push_back(gravelMaterial);

	// trunk material: dark brown bark, rough and mostly diffuse
	OBJECT_MATERIAL trunkMaterial;
	trunkMaterial.ambientColor = glm::vec3(0.2f, 0.1f, 0.05f);
	trunkMaterial.ambientStrength = 0.3f;
	trunkMaterial.diffuseColor = glm::vec3(0.4f, 0.25f, 0.1f);
	trunkMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	trunkMaterial.shininess = 4.0f;
	trunkMaterial.tag = "trunk_material";
	m_objectMaterials.push_back(trunkMaterial);

	/*** LIGHT SOURCES ***/
	// Light positions are NOT set here, they are re-uploaded every frame at the
	// top of RenderScene() so they remain anchored in world space and do not
	// follow the camera as the view matrix changes each frame.

	// enable Phong lighting calculations in the fragment shader



	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// light 0: warm white fill, moderate point light from the right side.
	// provides base illumination so the scene is not fully dark on that side.


	m_pShaderManager->setVec3Value("lightSources[0].ambientColor",
		glm::vec3(0.2f, 0.2f, 0.18f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor",
		glm::vec3(0.7f, 0.68f, 0.6f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor",
		glm::vec3(0.4f, 0.4f, 0.35f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.3f);

	// light 1: colored fill light, soft cool blue from the right.
	// satisfies the rubric requirement for at least one colored light source.
	// prevents shadow sides from going completely dark (open-sky ambient bounce).
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor",
		glm::vec3(0.05f, 0.08f, 0.15f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor",


		glm::vec3(0.2f, 0.3f, 0.5f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor",
		glm::vec3(0.1f, 0.15f, 0.3f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 8.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.15f);

	// light 2: primary sun, extremely high above the scene offset to the left.
	// at Y = 100 the angle from any object to the light barely changes as the
	// camera moves, so the "following" effect caused by view-space shading
	// becomes imperceptible. this is the main source of top-left sunlight.
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor",
		glm::vec3(0.35f, 0.33f, 0.28f));
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor",
		glm::vec3(1.0f, 0.97f, 0.88f));
	m_pShaderManager->setVec3Value("lightSources[2].specularColor",
		glm::vec3(0.9f, 0.88f, 0.75f));
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 64.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.6f);
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** RE-UPLOAD LIGHT POSITIONS EVERY FRAME ***/
	// Positions must be refreshed here each frame so they stay anchored to
	// fixed world-space coordinates. Setting them only in PrepareScene() causes
	// them to appear to follow the camera as the view matrix changes.

	// light 0: moderate warm fill, right side of scene

	m_pShaderManager->setVec3Value("lightSources[0].position",
		glm::vec3(10.0f, 8.0f, 5.0f));

	// light 1: soft cool blue fill, right-front, keeps shadow sides from going black
	m_pShaderManager->setVec3Value("lightSources[1].position",
		glm::vec3(6.0f, 5.0f, 10.0f));

	// light 2: primary sun, placed extremely high (-X, very high Y) so the
	// angle to any object barely shifts as the camera moves around the scene.
	// the negative X offset gives the top-left sun direction seen in the reference image.
	// at this altitude it functions like a distant directional light.
	m_pShaderManager->setVec3Value("lightSources[2].position",
		glm::vec3(-8.0f, 100.0f, 10.0f));

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	// gravel texture tiled so it repeats naturally across the wide plane
	// without appearing stretched
	SetShaderTexture("gravel");
	SetTextureUVScale(8.0f, 8.0f);
	// dull, granular Phong response for the gravel surface
	SetShaderMaterial("gravel_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// LEFT RECTANGULAR BOX

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.2f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.5f, 0.6f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// tile along the length of the long hedge face
	SetShaderTexture("hedge");
	SetTextureUVScale(4.0f, 1.0f);
	// matte foliage Phong response for all hedge surfaces
	SetShaderMaterial("hedge_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// short section joining the left hedge to the background
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 1.2f, 1.5f);

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 0.6f, -4.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("hedge");
	SetTextureUVScale(2.0f, 1.0f);
	SetShaderMaterial("hedge_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// RIGHT RECTANGULAR HEDGE BORDER 

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.2f, 8.0f);

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.5f, 0.6f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("hedge");
	SetTextureUVScale(4.0f, 1.0f);
	SetShaderMaterial("hedge_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 1.2f, 1.5f);

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.6f, -4.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("hedge");
	SetTextureUVScale(2.0f, 1.0f);
	SetShaderMaterial("hedge_material");

	m_basicMeshes->DrawBoxMesh();

	// CIRCULAR HEDGE RING

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 1.0f, 3.0f);

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -3.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// tile around the circumference of the ring
	SetShaderTexture("hedge");
	SetTextureUVScale(3.0f, 1.0f);
	SetShaderMaterial("hedge_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// LEFT TALL CONE TOPIARY

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.1f, 5.5f, 1.1f);

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.5f, 0.0f, -1.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("hedge");
	SetTextureUVScale(1.0f, 2.0f);
	SetShaderMaterial("hedge_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	// RIGHT LARGE SPHERE BUSH

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 4.0f, 2.5f);

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.5f, 0.0f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("hedge");
	SetTextureUVScale(2.0f, 2.0f);
	SetShaderMaterial("hedge_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	// CENTRAL TIERED TOPIARY

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 1.5f, 0.35f);

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -3.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// solid brown color for the visible trunk stem at the base of the tiered topiary
	SetShaderColor(0.36f, 0.22f, 0.11f, 1.0f);
	// bark-like Phong response: mostly diffuse, very low specular
	SetShaderMaterial("trunk_material");

	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.6f, 2.2f, 1.6f);

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.5f, -3.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// wider UV tiling for the broad base tier
	SetShaderTexture("hedge");
	SetTextureUVScale(2.0f, 1.5f);
	SetShaderMaterial("hedge_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.1f, 1.8f, 1.1f);

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.1f, -3.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// slightly tighter tiling for the narrower middle tier
	SetShaderTexture("hedge");
	SetTextureUVScale(1.5f, 1.5f);
	SetShaderMaterial("hedge_material");

	m_basicMeshes->DrawConeMesh();

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.65f, 1.4f, 0.65f);

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 4.5f, -3.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// tight tiling for the narrow tip
	SetShaderTexture("hedge");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("hedge_material");

	m_basicMeshes->DrawConeMesh();

	// BACKGROUND CONE TOPIARIES

	// background left cone
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 3.5f, 0.8f);

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.0f, -6.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("hedge");
	SetTextureUVScale(1.0f, 2.0f);
	SetShaderMaterial("hedge_material");

	m_basicMeshes->DrawConeMesh();

	// background right cone
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 3.5f, 0.8f);

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 0.0f, -6.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("hedge");
	SetTextureUVScale(1.0f, 2.0f);
	SetShaderMaterial("hedge_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	// BACKGROUND HEDGE WALL

	// set the XYZ scale for the mesh
	// height is double the side border hedges (1.2 * 2 = 2.4), wide enough to close the back
	scaleXYZ = glm::vec3(16.0f, 2.4f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	// Y is half the scale height so the base sits flush on the ground plane
	positionXYZ = glm::vec3(0.0f, 1.2f, -7.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// tile the texture across the wide face to avoid stretching
	SetShaderTexture("hedge");
	SetTextureUVScale(8.0f, 2.0f);
	SetShaderMaterial("hedge_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}