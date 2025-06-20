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
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
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
  *  DefineObjectMaterials() -MK
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Define materials for the objects in the scene -MK
	OBJECT_MATERIAL lampShadeMaterial;
	lampShadeMaterial.ambientColor = glm::vec3(0.6f, 0.6f, 0.4f);
	lampShadeMaterial.ambientStrength = 0.4f;
	lampShadeMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.7f);
	lampShadeMaterial.specularColor = glm::vec3(1.0f, 1.0f, 0.9f);
	lampShadeMaterial.shininess = 40.0;
	lampShadeMaterial.tag = "lampShade";
	m_objectMaterials.push_back(lampShadeMaterial);
}

/***********************************************************
 *  SetupSceneLights() -MK
 *
 *  This method is used for setting up the scene lights
 *  in the shader.  The light sources are defined in the
 *  shader code.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Light from above -MK
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 10.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", -1.05f, -1.05f, -1.02f); // subtle ambient light -MK
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.25f, 0.25f, 0.12f); // yellowish diffuse light -MK
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.9f, 0.9f, 0.8f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 25.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 3.0f);

	// Second Light -MK
	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 5.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.02f, 0.02f, 0.08f);   // subtle blue ambient -MK
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.2f, 0.2f, 0.9f);      // blue light -MK
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.6f, 0.6f, 1.0f);     // bluish highlights -MK
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 15.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 2.5f);

	// Prevents unused light sources from affecting the scene -MK
	// Was not able to move the light position without this, so this is a workaround -MK
	for (int i = 2; i < 4; ++i) {
		std::string base = "lightSources[" + std::to_string(i) + "]";
		m_pShaderManager->setVec3Value((base + ".position").c_str(), 0.0f, 7.0f, -7.0f);
		m_pShaderManager->setVec3Value((base + ".ambientColor").c_str(), 0.0f, 0.0f, 0.0f);
		m_pShaderManager->setVec3Value((base + ".diffuseColor").c_str(), 0.0f, 0.0f, 0.0f);
		m_pShaderManager->setVec3Value((base + ".specularColor").c_str(), 0.0f, 0.0f, 0.0f);
		m_pShaderManager->setFloatValue((base + ".focalStrength").c_str(), 0.0f);
		m_pShaderManager->setFloatValue((base + ".specularIntensity").c_str(), 0.0f);
	}
	
	// Enable lighting system -MK
	m_pShaderManager->setBoolValue(g_UseLightingName, true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{

	// define the materials for objects in the scene -MK
	DefineObjectMaterials();
	// add and define the light sources for the scene -MK
	SetupSceneLights();

	// Load scene geometry
	m_basicMeshes->LoadPlaneMesh();
	CreateGLTexture("Green_Mouse_Texture.jpg", "mouse");
	BindGLTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();

	// Load texture images and tag -MK
	CreateGLTexture("Green_Mouse_Texture.jpg", "mouse");

	// Load texture for lamp -MK
	CreateGLTexture("metal_Texture.jpg", "lamp");

	// Load textures for the desk -MK
	CreateGLTexture("wood.jpg", "desk");

	// Bind the loaded textures to OpenGL texture slots -MK
	BindGLTextures();
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

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(16.0f, 5.0f, 7.0f); // Adjust the scale for 3D desk -MK

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);

	SetShaderTexture("desk"); // Apply wood texture -MK

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	
	// ***START OF LAPTOP***

	// Load the box mesh for the laptop -MK
	m_basicMeshes->LoadBoxMesh(); 

	 // Laptop Base -MK
	glm::vec3 baseScale = glm::vec3(9.0f, 0.4f, 6.0f); // Adjust size -MK
	glm::vec3 basePosition = glm::vec3(0.0f, 0.2f, 0.0f); //Adjust position -MK

	SetTransformations(baseScale, 0.0f, 0.0f, 0.0f, basePosition);
	SetShaderColor(0.2f, 0.3f, 0.2f, 1.0f); // Dark green color -MK
	m_basicMeshes->DrawBoxMesh();

	// Laptop Screen -MK
	glm::vec3 screenScale = glm::vec3(9.0f, 6.0f, 0.2f); //Adjust size -MK
	glm::vec3 screenPosition = glm::vec3(0.0f, 3.2f, -2.6f); //Adjust position -MK

	SetTransformations(screenScale, -15.0f, 0.0f, 0.0f, screenPosition);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // Black screen frame -MK
	m_basicMeshes->DrawBoxMesh();

	// Laptop Screen Display -MK
	glm::vec3 displayScale = glm::vec3(8.4f, 5.4f, 0.1f); // Adjust size -MK
	glm::vec3 displayPosition = glm::vec3(0.0f, 3.2f, -2.64f); // Adjust position -MK

	SetTransformations(displayScale, -15.0f, 0.0f, 0.0f, displayPosition);
	SetShaderColor(0.1f, 0.3f, 0.8f, 1.0f); // Blue screen color -MK
	m_basicMeshes->DrawBoxMesh();

	// Small Touchpad -MK
	glm::vec3 touchpadScale = glm::vec3(1.6f, 0.1f, 1.2f); // Adjust size -MK
	glm::vec3 touchpadPosition = glm::vec3(0.0f, 0.44f, 1.6f); // Adjust position -MK

	SetTransformations(touchpadScale, 0.0f, 0.0f, 0.0f, touchpadPosition);
	SetShaderColor(0.3f, 0.3f, 0.3f, 1.0f); // Gray touchpad color -MK
	m_basicMeshes->DrawBoxMesh();

	// Keyboard -MK
	glm::vec3 keyboardScale = glm::vec3(7.0f, 0.1f, 2.4f); // Adjust size -MK
	glm::vec3 keyboardPosition = glm::vec3(0.0f, 0.44f, -1.0f); // Adjust position -MK

	SetTransformations(keyboardScale, 0.0f, 0.0f, 0.0f, keyboardPosition);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark colored keyboard -MK

	m_basicMeshes->DrawBoxMesh();

	// ***END OF LAPTOP***


	// ***START OF MOUSE***

	// Load a sphere for the mouse shape -MK
	m_basicMeshes->LoadSphereMesh();

	// Mouse body -MK
	glm::vec3 mouseScale = glm::vec3(1.2f, 0.6f, 2.0f);  // Adjust size -MK
	glm::vec3 mousePosition = glm::vec3(7.0f, 0.35f, 2.0f); // Adjust position -MK

	SetTransformations(mouseScale, 0.0f, 0.0f, 0.0f, mousePosition);
	SetShaderTexture("mouse"); // Apply green texture -MK
	m_basicMeshes->DrawSphereMesh();

	// ***END OF MOUSE***

	// *** START OF LAMP***
	
	// preload -MK
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();

	// Lamp BASE -MK

	glm::vec3 lampBaseScale = glm::vec3(2.0f, 0.2f, 2.0f);         
	glm::vec3 lampBasePosition = glm::vec3(-12.0f, 0.1f, 0.0f);    
	SetTransformations(lampBaseScale, 0.0f, 0.0f, 0.0f, lampBasePosition);
	SetShaderColor(0.25f, 0.25f, 0.25f, 1.0f); // gray color -MK
	m_basicMeshes->DrawCylinderMesh();

	// Lamp Stand -MK

	glm::vec3 lampStandScale = glm::vec3(0.25f, 6.0f, 0.25f); 
	glm::vec3 lampStandPosition = glm::vec3(-12.0f, 0.7, 0.0f);
	SetTransformations(lampStandScale, 0.0f, 0.0f, 0.0f, lampStandPosition);
	SetShaderColor(0.30f, 0.30f, 0.30f, 1.0f); // slightly lighter gray -MK

	SetShaderTexture("lamp"); // Apply metal texture -MK
	m_basicMeshes->DrawCylinderMesh();

	// Lamp Shade -MK

	glm::vec3 lampShadeScale = glm::vec3(2.0, 1.5, 3.0);
	glm::vec3 lampShadePosition = glm::vec3(-12.0, 7.9, 0.0);
	SetTransformations(lampShadeScale, 10.0, 0.0, 125.0, lampShadePosition);
	SetShaderColor(0.85, 0.85, 0.7, 1.0); // light cream color -MK

	SetShaderMaterial("lampShade"); // Use defined material for lamp shade -MK
	m_basicMeshes->DrawConeMesh();
	
	m_basicMeshes->DrawConeMesh();
	m_basicMeshes->DrawSphereMesh();

	// *** END OF LAMP ***

	// *** START OF BOOK STACK *** -MK
	m_basicMeshes->LoadBoxMesh();

	// Bottom book -MK
	glm::vec3 book1Scale = glm::vec3(3.5f, 0.6f, 2.5f);
	glm::vec3 book1Position = glm::vec3(8.0f, 0.3f, -3.2f);

	SetTransformations(book1Scale, 0.0f, 5.0f, 0.0f, book1Position);
	SetShaderColor(0.0f, 0.5f, 0.0f, 1.0f); // green color -MK
	m_basicMeshes->DrawBoxMesh();

	// Top book -MK
	glm::vec3 book2Scale = glm::vec3(3.5f, 0.6f, 2.5f);
	glm::vec3 book2Position = glm::vec3(8.0f, 0.9f, -3.2f);

	SetTransformations(book2Scale, 0.0f, -5.0f, 0.0f, book2Position);
	SetShaderColor(0.1f, 0.1f, 0.6f, 1.0f); // blue color -MK
	m_basicMeshes->DrawBoxMesh();

	// *** END OF BOOK STACK ***

	// *** START OF COFFEE MUG *** - MK
	m_basicMeshes->LoadCylinderMesh();

	// Mug body -MK
	glm::vec3 mugScale = glm::vec3(0.8f, 1.2f, 0.8f);
	glm::vec3 mugPosition = glm::vec3(-7.5f, 0.6f, 2.5f); // Front left of desk -MK

	SetTransformations(mugScale, 0.0f, 0.0f, 0.0f, mugPosition);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White mug -MK
	m_basicMeshes->DrawCylinderMesh();

	// Mug top -MK
	glm::vec3 mugTopScale = glm::vec3(0.78f, 0.05f, 0.78f);
	glm::vec3 mugTopPosition = glm::vec3(-7.5f, 1.2f, 2.5f); // Top of mug -MK

	SetTransformations(mugTopScale, 0.0f, 0.0f, 0.0f, mugTopPosition);
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f); // Dark gray top -MK
	m_basicMeshes->DrawCylinderMesh();

	// *** END OF COFFEE MUG ***
}
