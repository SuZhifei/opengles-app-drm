/******************************************************************************

 @File         OGLES2PVRScopeExample.cpp

 @Title        Iridescence

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     Independent

 @Description  Shows how to use our example PVRScope graph code.

******************************************************************************/
#include "PVRShell.h"
#include "OGLES2Tools.h"
#include "PVRScopeGraph.h"

#if !defined(_WIN32) || defined(__WINSCW__)
#define _stricmp strcasecmp
#endif

/******************************************************************************
 Defines
******************************************************************************/

// Camera constants. Used for making the projection matrix
#define CAM_NEAR	(1.0f)
#define CAM_FAR		(5000.0f)

// Index to bind the attributes to vertex shaders
#define VERTEX_ARRAY	0
#define NORMAL_ARRAY	1
#define TEXCOORD_ARRAY	2

/******************************************************************************
 Content file names
******************************************************************************/

// Source and binary shaders
const char c_szFragShaderSrcFile[]	= "FragShader.fsh";
const char c_szFragShaderBinFile[]	= "FragShader.fsc";
const char c_szVertShaderSrcFile[]	= "VertShader.vsh";
const char c_szVertShaderBinFile[]	= "VertShader.vsc";

// PVR texture files
const char c_szTextureFile[]		= "Thickness.pvr";

// POD scene files
const char c_szSceneFile[]			= "Mask.pod";

/*!****************************************************************************
 Class implementing the PVRShell functions.
******************************************************************************/
class OGLES2PVRScopeExample : public PVRShell
{
	// Print3D class used to display text
	CPVRTPrint3D	m_Print3D;

	// 3D Model
	CPVRTModelPOD	m_Scene;

	// Projection and view matrices
	PVRTMat4		m_mProjection, m_mView;

	// OpenGL handles for shaders, textures and VBOs
	GLuint m_uiVertShader;
	GLuint m_uiFragShader;
	GLuint m_uiTexture;
	GLuint* m_puiVbo;
	GLuint* m_puiIndexVbo;

	// Group shader programs and their uniform locations together
	struct
	{
		GLuint uiId;
		GLuint uiMVPMatrixLoc;
		GLuint uiLightDirLoc;
		GLuint uiEyePosLoc;
		GLuint uiMinThicknessLoc;
		GLuint uiMaxVariationLoc;
	}
	m_ShaderProgram;

	// The translation and Rotate parameter of Model
	float m_fAngleY;

	float m_fMinThickness;
	float m_fMaxVariation;

	// The PVRScopeGraph variable
	CPVRScopeGraph *m_pScopeGraph;

	// Variables for the graphing code
	int m_i32Counter;
	int m_i32Group;
	int m_i32Interval;

public:
	virtual bool InitApplication();
	virtual bool InitView();
	virtual bool ReleaseView();
	virtual bool QuitApplication();
	virtual bool RenderScene();

	bool LoadTextures(CPVRTString* pErrorStr);
	bool LoadShaders(CPVRTString* pErrorStr);
	void LoadVbos();

	void DrawMesh(int i32NodeIndex);
};

/*!****************************************************************************
 @Function		LoadTextures
 @Output		pErrorStr		A string describing the error on failure
 @Return		bool			true if no error occured
 @Description	Loads the textures required for this training course
******************************************************************************/
bool OGLES2PVRScopeExample::LoadTextures(CPVRTString* const pErrorStr)
{
	if(PVRTTextureLoadFromPVR(c_szTextureFile, &m_uiTexture) != PVR_SUCCESS)
	{
		*pErrorStr = "ERROR: Failed to load texture.";
		return false;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return true;
}

/*!****************************************************************************
 @Function		LoadShaders
 @Output		pErrorStr		A string describing the error on failure
 @Return		bool			true if no error occured
 @Description	Loads and compiles the shaders and links the shader programs
				required for this training course
******************************************************************************/
bool OGLES2PVRScopeExample::LoadShaders(CPVRTString* pErrorStr)
{
	/*
		Load and compile the shaders from files.
		Binary shaders are tried first, source shaders
		are used as fallback.
	*/
	if (PVRTShaderLoadFromFile(
			c_szVertShaderBinFile, c_szVertShaderSrcFile, GL_VERTEX_SHADER, GL_SGX_BINARY_IMG, &m_uiVertShader, pErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	if (PVRTShaderLoadFromFile(
			c_szFragShaderBinFile, c_szFragShaderSrcFile, GL_FRAGMENT_SHADER, GL_SGX_BINARY_IMG, &m_uiFragShader, pErrorStr) != PVR_SUCCESS)
	{
		return false;
	}

	/*
		Set up and link the shader program
	*/
	const char* aszAttribs[] = { "inVertex", "inNormal", "inTexCoord" };
	if (PVRTCreateProgram(&m_ShaderProgram.uiId, m_uiVertShader, m_uiFragShader, aszAttribs, 3, pErrorStr))
	{
		PVRShellSet(prefExitMessage, pErrorStr->c_str());
		return false;
	}
	// Set the sampler2D variable to the first texture unit
	glUniform1i(glGetUniformLocation(m_ShaderProgram.uiId, "sThicknessTex"), 0);

	// Store the location of uniforms for later use
	m_ShaderProgram.uiMVPMatrixLoc		= glGetUniformLocation(m_ShaderProgram.uiId, "MVPMatrix");
	m_ShaderProgram.uiLightDirLoc		= glGetUniformLocation(m_ShaderProgram.uiId, "LightDirection");
	m_ShaderProgram.uiEyePosLoc			= glGetUniformLocation(m_ShaderProgram.uiId, "EyePosition");
	m_ShaderProgram.uiMinThicknessLoc	= glGetUniformLocation(m_ShaderProgram.uiId, "MinThickness");
	m_ShaderProgram.uiMaxVariationLoc	= glGetUniformLocation(m_ShaderProgram.uiId, "MaxVariation");

	return true;
}

/*!****************************************************************************
 @Function		LoadVbos
 @Description	Loads the mesh data required for this training course into
				vertex buffer objects
******************************************************************************/
void OGLES2PVRScopeExample::LoadVbos()
{
	if (!m_puiVbo)      m_puiVbo = new GLuint[m_Scene.nNumMesh];
	if (!m_puiIndexVbo) m_puiIndexVbo = new GLuint[m_Scene.nNumMesh];

	/*
		Load vertex data of all meshes in the scene into VBOs

		The meshes have been exported with the "Interleave Vectors" option,
		so all data is interleaved in the buffer at pMesh->pInterleaved.
		Interleaving data improves the memory access pattern and cache efficiency,
		thus it can be read faster by the hardware.
	*/
	glGenBuffers(m_Scene.nNumMesh, m_puiVbo);
	for (unsigned int i = 0; i < m_Scene.nNumMesh; ++i)
	{
		// Load vertex data into buffer object
		SPODMesh& Mesh = m_Scene.pMesh[i];
		unsigned int uiSize = Mesh.nNumVertex * Mesh.sVertex.nStride;
		glBindBuffer(GL_ARRAY_BUFFER, m_puiVbo[i]);
		glBufferData(GL_ARRAY_BUFFER, uiSize, Mesh.pInterleaved, GL_STATIC_DRAW);

		// Load index data into buffer object if available
		m_puiIndexVbo[i] = 0;
		if (Mesh.sFaces.pData)
		{
			glGenBuffers(1, &m_puiIndexVbo[i]);
			uiSize = PVRTModelPODCountIndices(Mesh) * sizeof(GLshort);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_puiIndexVbo[i]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, uiSize, Mesh.sFaces.pData, GL_STATIC_DRAW);
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

/*!****************************************************************************
 @Function		InitApplication
 @Return		bool		true if no error occured
 @Description	Code in InitApplication() will be called by PVRShell once per
				run, before the rendering context is created.
				Used to initialize variables that are not dependant on it
				(e.g. external modules, loading meshes, etc.)
				If the rendering context is lost, InitApplication() will
				not be called again.
******************************************************************************/
bool OGLES2PVRScopeExample::InitApplication()
{
	m_puiVbo = 0;
	m_puiIndexVbo = 0;

	// At the time of writing, this counter is the USSE load for vertex + pixel processing
	m_i32Counter	= 46;
	m_i32Group		= 0;
	m_i32Interval	= 0;

	// Get and set the read path for content files
	CPVRTResourceFile::SetReadPath((char*)PVRShellGet(prefReadPath));

	// Get and set the load/release functions for loading external files.
	// In the majority of cases the PVRShell will return NULL function pointers implying that
	// nothing special is required to load external files.
	CPVRTResourceFile::SetLoadReleaseFunctions(PVRShellGet(prefLoadFileFunc), PVRShellGet(prefReleaseFileFunc));

	// Load the scene
	if (m_Scene.ReadFromFile(c_szSceneFile) != PVR_SUCCESS)
	{
		PVRShellSet(prefExitMessage, "ERROR: Couldn't load the .pod file\n");
		return false;
	}

	// set angle of rotation
	m_fAngleY = 0.0f;

	// Process the command line
	{
		const unsigned int	nOptNum			= PVRShellGet(prefCommandLineOptNum);
		const SCmdLineOpt	* const psOpt	= (const SCmdLineOpt*)PVRShellGet(prefCommandLineOpts);
		for(unsigned int i = 0; i < nOptNum; ++i)
		{
			if(_stricmp(psOpt[i].pArg, "-counter") == 0 && psOpt[i].pVal)
			{
				m_i32Counter = atoi(psOpt[i].pVal);
			}
			else if(_stricmp(psOpt[i].pArg, "-group") == 0 && psOpt[i].pVal)
			{
				m_i32Group = atoi(psOpt[i].pVal);
			}
			else if(_stricmp(psOpt[i].pArg, "-interval") == 0 && psOpt[i].pVal)
			{
				m_i32Interval = atoi(psOpt[i].pVal);
			}
		}
	}

	return true;
}

/*!****************************************************************************
 @Function		QuitApplication
 @Return		bool		true if no error occured
 @Description	Code in QuitApplication() will be called by PVRShell once per
				run, just before exiting the program.
				If the rendering context is lost, QuitApplication() will
				not be called.x
******************************************************************************/
bool OGLES2PVRScopeExample::QuitApplication()
{
	// Free the memory allocated for the scene
	m_Scene.Destroy();

	delete [] m_puiVbo;
	delete [] m_puiIndexVbo;

    return true;
}

/*!****************************************************************************
 @Function		InitView
 @Return		bool		true if no error occured
 @Description	Code in InitView() will be called by PVRShell upon
				initialization or after a change in the rendering context.
				Used to initialize variables that are dependant on the rendering
				context (e.g. textures, vertex buffers, etc.)
******************************************************************************/
bool OGLES2PVRScopeExample::InitView()
{
	CPVRTString ErrorStr;

	/*
		Initialize VBO data
	*/
	LoadVbos();

	/*
		Load textures
	*/
	if (!LoadTextures(&ErrorStr))
	{
		PVRShellSet(prefExitMessage, ErrorStr.c_str());
		return false;
	}

	/*
		Load and compile the shaders & link programs
	*/
	if (!LoadShaders(&ErrorStr))
	{
		PVRShellSet(prefExitMessage, ErrorStr.c_str());
		return false;
	}

	// Is the screen rotated?
	bool bRotate = PVRShellGet(prefIsRotated) && PVRShellGet(prefFullScreen);

	/*
		Initialize Print3D
	*/
	if(m_Print3D.SetTextures(0,PVRShellGet(prefWidth),PVRShellGet(prefHeight), bRotate) != PVR_SUCCESS)
	{
		PVRShellSet(prefExitMessage, "ERROR: Cannot initialise Print3D\n");
		return false;
	}

	/*
		Calculate the projection and view matrices
	*/

	m_mProjection = PVRTMat4::PerspectiveFovRH(PVRT_PI/6, (float)PVRShellGet(prefWidth)/(float)PVRShellGet(prefHeight), CAM_NEAR, CAM_FAR, PVRTMat4::OGL, bRotate);

	m_mView = PVRTMat4::LookAtRH(PVRTVec3(0, 0, 75), PVRTVec3(0, 0, 0), PVRTVec3(0, 1, 0));

	/*
		Set OpenGL ES render states needed for this training course
	*/
	// Enable backface culling and depth test
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);

	// Use a nice bright blue as clear colour
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f);

	// set thickness variation of the film
	m_fMaxVariation	= 100.0f;
	// set the minimum thickness of the film
	m_fMinThickness = 100.0f;

	// Initialise the graphing code
	m_pScopeGraph = new CPVRScopeGraph();

	if(m_pScopeGraph)
	{
		// Position the graph
		m_pScopeGraph->position(PVRShellGet(prefWidth), PVRShellGet(prefHeight), (int) (PVRShellGet(prefWidth) * 0.02f), (int) (PVRShellGet(prefHeight) * 0.02f), (int) (PVRShellGet(prefWidth) * 0.96f), (int) (PVRShellGet(prefHeight) * 0.96f) / 3);

		// Output the current active group and a list of all the counters
		PVRShellOutputDebug("Active Group %i\nCounter Number %i\n", m_pScopeGraph->GetActiveGroup(), m_pScopeGraph->GetCounterNum());
		PVRShellOutputDebug("Counters\n");

		for(unsigned int i = 0; i < m_pScopeGraph->GetCounterNum(); ++i)
		{
			PVRShellOutputDebug("(%i) Name %s Group %i %s\n", i,
				m_pScopeGraph->GetCounterName(i), m_pScopeGraph->GetCounterGroup(i),
				m_pScopeGraph->IsCounterPercentage(i) ? "percentage" : "absolute");
			m_pScopeGraph->ShowCounter(i, false);
		}

		// Set the active group to 0
		m_pScopeGraph->SetActiveGroup(m_i32Group);

		// Tell the graph to show an initial counter
		m_pScopeGraph->ShowCounter(m_i32Counter, true);

		// Set the update interval: number of updates [frames] before updating the graph
		m_pScopeGraph->SetUpdateInterval(m_i32Interval);
	}

	return true;
}

/*!****************************************************************************
 @Function		ReleaseView
 @Return		bool		true if no error occured
 @Description	Code in ReleaseView() will be called by PVRShell when the
				application quits or before a change in the rendering context.
******************************************************************************/
bool OGLES2PVRScopeExample::ReleaseView()
{
	// Delete textures
	glDeleteTextures(1, &m_uiTexture);

	// Delete program and shader objects
	glDeleteProgram(m_ShaderProgram.uiId);

	glDeleteShader(m_uiVertShader);
	glDeleteShader(m_uiFragShader);

	// Delete buffer objects
	glDeleteBuffers(m_Scene.nNumMesh, m_puiVbo);
	glDeleteBuffers(m_Scene.nNumMesh, m_puiIndexVbo);

	// Release Print3D Textures
	m_Print3D.ReleaseTextures();

	if(m_pScopeGraph)
	{
		delete m_pScopeGraph;
		m_pScopeGraph = 0;
	}

	return true;
}

/*!****************************************************************************
 @Function		RenderScene
 @Return		bool		true if no error occured
 @Description	Main rendering loop function of the program. The shell will
				call this function every frame.
				eglSwapBuffers() will be performed by PVRShell automatically.
				PVRShell will also manage important OS events.
				Will also manage relevent OS events. The user has access to
				these events through an abstraction layer provided by PVRShell.
******************************************************************************/
bool OGLES2PVRScopeExample::RenderScene()
{
	// Keyboard input (cursor up/down to cycle through counters)
	if(PVRShellIsKeyPressed(PVRShellKeyNameUP))
	{
		m_i32Counter++;

		if(m_i32Counter > (int) m_pScopeGraph->GetCounterNum())
			m_i32Counter = m_pScopeGraph->GetCounterNum();
	}

	if(PVRShellIsKeyPressed(PVRShellKeyNameDOWN))
	{
		m_i32Counter--;

		if(m_i32Counter < 0)
			m_i32Counter = 0;
	}

	if(PVRShellIsKeyPressed(PVRShellKeyNameACTION2))
		m_pScopeGraph->ShowCounter(m_i32Counter, !m_pScopeGraph->IsCounterShown(m_i32Counter));

	// Keyboard input (cursor left/right to change active group)
	if(PVRShellIsKeyPressed(PVRShellKeyNameRIGHT))
	{
		m_pScopeGraph->SetActiveGroup(m_pScopeGraph->GetActiveGroup()+1);
	}

	if(PVRShellIsKeyPressed(PVRShellKeyNameLEFT))
	{
		m_pScopeGraph->SetActiveGroup(m_pScopeGraph->GetActiveGroup()-1);
	}

	// Clear the color and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use shader program
	glUseProgram(m_ShaderProgram.uiId);

	// Bind texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_uiTexture);

	// Rotate and Translation the model matrix
	PVRTMat4 mModel;
	mModel = PVRTMat4::RotationY(m_fAngleY);
	m_fAngleY += (2*PVRT_PI/60)/7;

	// Set model view projection matrix
	PVRTMat4 mModelView, mMVP;
	mModelView = m_mView * mModel;
	mMVP =  m_mProjection * mModelView;
	glUniformMatrix4fv(m_ShaderProgram.uiMVPMatrixLoc, 1, GL_FALSE, mMVP.ptr());

	// Set light direction in model space
	PVRTVec4 vLightDirModel;
	vLightDirModel = mModel.inverse() * PVRTVec4(1, 1, 1, 0);

	glUniform3fv(m_ShaderProgram.uiLightDirLoc, 1, &vLightDirModel.x);

	// Set eye position in model space
	PVRTVec4 vEyePosModel;
	vEyePosModel = mModelView.inverse() * PVRTVec4(0, 0, 0, 1);
	glUniform3fv(m_ShaderProgram.uiEyePosLoc, 1, &vEyePosModel.x);

	/*
		Set the iridescent shading parameters
	*/
	// Set the minimum thickness of the coating in nm
	glUniform1f(m_ShaderProgram.uiMinThicknessLoc, m_fMinThickness);

	// Set the maximum variation in thickness of the coating in nm
	glUniform1f(m_ShaderProgram.uiMaxVariationLoc, m_fMaxVariation);

	/*
		Now that the uniforms are set, call another function to actually draw the mesh.
	*/
	DrawMesh(0);

	char Description[256];

	if(m_pScopeGraph->GetCounterNum())
	{
		sprintf(Description, "Active Grp %i\n\nCounter %i (Grp %i) \nName: %s\nShown: %s\nuser y-axis: %.2f  max: %.2f%s",
			m_pScopeGraph->GetActiveGroup(), m_i32Counter,
			m_pScopeGraph->GetCounterGroup(m_i32Counter),
			m_pScopeGraph->GetCounterName(m_i32Counter),
			m_pScopeGraph->IsCounterShown(m_i32Counter) ? "Yes" : "No",
			m_pScopeGraph->GetMaximum(m_i32Counter),
			m_pScopeGraph->GetMaximumOfData(m_i32Counter),
			m_pScopeGraph->IsCounterPercentage(m_i32Counter) ? "%%" : "");
	}
	else
	{
		sprintf(Description, "No counters present");
	}

	// Displays the demo name using the tools. For a detailed explanation, see the training course IntroducingPVRTools
	m_Print3D.DisplayDefaultTitle("PVRScopeExample", Description, ePVRTPrint3DSDKLogo);
	m_Print3D.Flush();

	// Update counters and draw the graph
	m_pScopeGraph->Ping(PVRShellGetTime() * 1000);

	return true;
}

/*!****************************************************************************
 @Function		DrawMesh
 @Input			i32NodeIndex		Node index of the mesh to draw
 @Description	Draws a SPODMesh after the model view matrix has been set and
				the meterial prepared.
******************************************************************************/
void OGLES2PVRScopeExample::DrawMesh(int i32NodeIndex)
{
	int i32MeshIndex = m_Scene.pNode[i32NodeIndex].nIdx;
	SPODMesh* pMesh = &m_Scene.pMesh[i32MeshIndex];

	// bind the VBO for the mesh
	glBindBuffer(GL_ARRAY_BUFFER, m_puiVbo[i32MeshIndex]);
	// bind the index buffer, won't hurt if the handle is 0
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_puiIndexVbo[i32MeshIndex]);

	// Enable the vertex attribute arrays
	glEnableVertexAttribArray(VERTEX_ARRAY);
	glEnableVertexAttribArray(NORMAL_ARRAY);
	glEnableVertexAttribArray(TEXCOORD_ARRAY);

	// Set the vertex attribute offsets
	glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, pMesh->sVertex.nStride, pMesh->sVertex.pData);
	glVertexAttribPointer(NORMAL_ARRAY, 3, GL_FLOAT, GL_FALSE, pMesh->sNormals.nStride, pMesh->sNormals.pData);
	glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, pMesh->psUVW[0].nStride, pMesh->psUVW[0].pData);

	/*
		The geometry can be exported in 4 ways:
		- Indexed Triangle list
		- Non-Indexed Triangle list
		- Indexed Triangle strips
		- Non-Indexed Triangle strips
	*/
	if(pMesh->nNumStrips == 0)
	{
		if(m_puiIndexVbo[i32MeshIndex])
		{
			// Indexed Triangle list
			glDrawElements(GL_TRIANGLES, pMesh->nNumFaces*3, GL_UNSIGNED_SHORT, 0);
		}
		else
		{
			// Non-Indexed Triangle list
			glDrawArrays(GL_TRIANGLES, 0, pMesh->nNumFaces*3);
		}
	}
	else
	{
		for(int i = 0; i < (int)pMesh->nNumStrips; ++i)
		{
			int offset = 0;
			if(m_puiIndexVbo[i32MeshIndex])
			{
				// Indexed Triangle strips
				glDrawElements(GL_TRIANGLE_STRIP, pMesh->pnStripLength[i]+2, GL_UNSIGNED_SHORT, (GLshort*)(offset*2));
			}
			else
			{
				// Non-Indexed Triangle strips
				glDrawArrays(GL_TRIANGLE_STRIP, offset, pMesh->pnStripLength[i]+2);
			}
			offset += pMesh->pnStripLength[i]+2;
		}
	}

	// Safely disable the vertex attribute arrays
	glDisableVertexAttribArray(VERTEX_ARRAY);
	glDisableVertexAttribArray(NORMAL_ARRAY);
	glDisableVertexAttribArray(TEXCOORD_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

/*!****************************************************************************
 @Function		NewDemo
 @Return		PVRShell*		The demo supplied by the user
 @Description	This function must be implemented by the user of the shell.
				The user should return its PVRShell object defining the
				behaviour of the application.
******************************************************************************/
PVRShell* NewDemo()
{
	return new OGLES2PVRScopeExample();
}

/******************************************************************************
 End of file (OGLES2PVRScopeExample.cpp)
******************************************************************************/

