/*
===========================================================================
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_glsl.c

#include "tr_local.h"

static char *complieMacroNames[] =
{
#define MACRO_NAME
#include "tr_glsldef.h"
#undef MACRO_NAME
};

// These must be in the same order as in uniform_t in tr_local.h.
static uniformInfo_t uniformsInfo[] =
{
#define UNIFORM_TYPE
#include "tr_glsldef.h"
#undef UNIFORM_TYPE
};

typedef struct
{
	unsigned int attr;
	char name[32];
} attrmap;

const attrmap attributeMap[] =
{
#define ATTR_MAP
#include "tr_glsldef.h"
#undef ATTR_MAP
};
const int numberofAttributes = ARRAY_LEN(attributeMap);

#define FILE_HASH_SIZE      4096
#define MAX_SHADER_DEF_FILES 1024
#define DEFAULT_SHADER_DEF NULL
static programInfo_t *hashTable[FILE_HASH_SIZE];

static char *definitionText;

#define GL_SHADER_VERSION 3

typedef struct GLShaderHeader_s
{
	unsigned int version;
	unsigned int checkSum; // checksum of shader source this was built from

	unsigned int macros[MAX_MACROS];   // macros the shader uses ( may or may not be enabled )
	unsigned int numMacros;

	GLenum binaryFormat; // argument to glProgramBinary
	GLint binaryLength;  // argument to glProgramBinary
}GLShaderHeader_t;

programInfo_t *gl_genericShader;
programInfo_t *gl_lightMappingShader;
programInfo_t *gl_vertexLightingShader_DBS_entity;
programInfo_t *gl_vertexLightingShader_DBS_world;
programInfo_t *gl_forwardLightingShader_omniXYZ;
programInfo_t *gl_forwardLightingShader_projXYZ;
programInfo_t *gl_forwardLightingShader_directionalSun;
programInfo_t *gl_deferredLightingShader_omniXYZ;
programInfo_t *gl_deferredLightingShader_projXYZ;
programInfo_t *gl_deferredLightingShader_directionalSun;
programInfo_t *gl_geometricFillShader;
programInfo_t *gl_shadowFillShader;
programInfo_t *gl_reflectionShader;
programInfo_t *gl_skyboxShader;
programInfo_t *gl_fogQuake3Shader;
programInfo_t *gl_fogGlobalShader;
programInfo_t *gl_heatHazeShader;
programInfo_t *gl_screenShader;
programInfo_t *gl_portalShader;
programInfo_t *gl_toneMappingShader;
programInfo_t *gl_contrastShader;
programInfo_t *gl_cameraEffectsShader;
programInfo_t *gl_blurXShader;
programInfo_t *gl_blurYShader;
programInfo_t *gl_debugShadowMapShader;

//Dushan
programInfo_t *gl_liquidShader;
programInfo_t *gl_rotoscopeShader;
programInfo_t *gl_bloomShader;
programInfo_t *gl_refractionShader;
programInfo_t *gl_depthToColorShader;
programInfo_t *gl_volumetricFogShader;
programInfo_t *gl_volumetricLightingShader;
programInfo_t *gl_dispersionShader;

programInfo_t *gl_depthOfField;
programInfo_t *gl_ssao;

//Jacker
programInfo_t *gl_colorCorrection;

//This is set with the GLSL_SelectPermutation
shaderProgram_t *selectedProgram;

/*
================
return a hash value for the filename
This function is cloned to many files, should be moved to common->
================
*/
static long GLSL_GenerateHashValue(const char *fname)
{
	int  i    = 0;
	long hash = 0;
	char letter;

	while (fname[i] != '\0')
	{
		letter = tolower(fname[i]);
		if (letter == '.')
		{
			break;                          // don't include extension
		}
		if (letter == PATH_SEP)
		{
			letter = '/';                   // damn path names
		}
		hash += (long)(letter) * (i + 119);
		i++;
	}
	hash &= (FILE_HASH_SIZE - 1);
	return hash;
}

int GLSL_GetMacroByName(const char *name)
{
	int i;
	for (i = 0; i < MAX_MACROS; i++)
	{
		if (!Q_stricmp(name, complieMacroNames[i]))
		{
			return i;
		}
	}

	return -1;
}

unsigned int GLSL_GetAttribByName(const char *name)
{
	int i;
	for (i = 0; i < numberofAttributes; i++)
	{
		if (!Q_stricmp(name, attributeMap[i].name))
		{
			return attributeMap[i].attr;
		}
	}

	return -1;
}

void GLSL_CopyStringAlloc(char **out, const char *in)
{
	size_t size = strlen(in) * sizeof(char) + 1;
	*out = (char *) Com_Allocate(size);
	Com_Memset(*out, '\0', size);
	Q_strncpyz(*out, in, size);
}

qboolean GLSL_CopyNextToken(char **text, char **out)
{
	char *token;
	token = COM_ParseExt(text, qtrue);
	if (!token[0])
	{
		return qfalse;
	}

	// end of shader definition
	if (token[0] == '}')
	{
		return qfalse;
	}

	GLSL_CopyStringAlloc(out, token);
	return qtrue;
}

programInfo_t *GLSL_ParseDefinition(char **text, const char *defname)
{
	char          *token;
	programInfo_t *def;
	void          *valptr;

	token = COM_ParseExt(text, qtrue);
	if (token[0] != '{')
	{
		ri.Printf(PRINT_WARNING, "WARNING: expecting '{', found '%s' instead in shader definition '%s'\n", token, defname);
		return NULL;
	}

	def = (programInfo_t *)Com_Allocate(sizeof(programInfo_t));
	Com_Memset(def, 0, sizeof(programInfo_t));

	GLSL_CopyStringAlloc(&def->name, defname);
	def->compiled = qfalse;

	while (1)
	{
		token = COM_ParseExt(text, qtrue);
		if (!token[0])
		{
			ri.Printf(PRINT_WARNING, "WARNING: no concluding '}' in shader definition %s\n", defname);
			goto parseerror;
		}

		// end of shader definition
		if (token[0] == '}')
		{
			break;
		}
		else if (!Q_stricmp(token, "filename"))
		{
			GLSL_CopyNextToken(text, &def->filename);
		}
		else if (!Q_stricmp(token, "fragfilename"))
		{
			GLSL_CopyNextToken(text, &def->fragFilename);
		}
		else if (!Q_stricmp(token, "macros"))
		{
			int macro;

			while ((token = COM_ParseExt(text, qfalse))[0])
			{
				macro = GLSL_GetMacroByName(token);
				if (macro >= 0)
				{
					def->macros[def->numMacros] = macro;
					def->numMacros++;
				}
				else
				{
					ri.Error(ERR_FATAL, "PERKELE: %s", token);
					ri.Printf(PRINT_WARNING, "WARNING: Macro '%s' for shaderdef '%s' was not recognized\n", token, defname);
					goto parseerror;
				}
			}
		}
		else if (!Q_stricmp(token, "extramacros"))
		{
			GLSL_CopyNextToken(text, &def->extraMacros);
		}
		else if (!Q_stricmp(token, "attribs"))
		{
			unsigned int attribs = 0;

			while ((token = COM_ParseExt(text, qfalse))[0])
			{
				attribs |= GLSL_GetAttribByName(token);
			}
			def->attributes = attribs;
		}
		else if (!Q_stricmp(token, "vertexLibraries"))
		{
			GLSL_CopyNextToken(text, &def->vertexLibraries);
		}
		else if (!Q_stricmp(token, "fragmentLibraries"))
		{
			GLSL_CopyNextToken(text, &def->fragmentLibraries);
		}
		else if (!Q_stricmp(token, "uniform"))
		{
			token = COM_ParseExt(text, qtrue);
			if (!Q_stricmp(token, "int"))
			{
				def->uniformValues[def->numUniformValues].type.type = GLSL_INT;
				GLSL_CopyNextToken(text, &def->uniformValues[def->numUniformValues].type.name);
				token                                           = COM_ParseExt(text, qtrue);
				valptr                                          = Com_Allocate(sizeof(int));
				*((int *)valptr)                                = atoi(token);
				def->uniformValues[def->numUniformValues].value = valptr;
				//Com_Printf("%d\n",*((int*)valptr));
			}
			//FIXME: implement other formats
			def->numUniformValues++;
		}
	}


	return def;

parseerror:
	Com_Dealloc(def);
	return NULL;
}

static char *GLSL_FindDefinitionInText(const char *shadername)
{
	char *p = definitionText;
	char *token;

	if (!p)
	{
		ri.Error(ERR_FATAL, "GLSL_FindDefinitionInText: Definition text is null");
	}

	// look for label
	// note that this could get confused if a shader name is used inside
	// another shader definition
	while (1)
	{
		token = COM_ParseExt(&p, qtrue);
		if (token[0] == 0)
		{
			break;
		}

		if (!Q_stricmp(token, shadername))
		{
			return p;
		}

		SkipBracedSection(&p);
	}

	return NULL;
}

programInfo_t *GLSL_FindShader(const char *name)
{
	char          strippedName[MAX_QPATH];
	int           hash;
	char          *shaderText;
	programInfo_t *sh;

	if (name[0] == 0)
	{
		return DEFAULT_SHADER_DEF;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));
	COM_FixPath(strippedName);

	hash = GLSL_GenerateHashValue(strippedName);

	for (sh = hashTable[hash]; sh; sh = sh->next)
	{
		// index by name
		// the original way was correct
		if (!Q_stricmp(sh->name, strippedName))
		{
			// match found
			return sh;
		}
	}

	shaderText = GLSL_FindDefinitionInText(strippedName);
	if (!shaderText)
	{
		ri.Printf(PRINT_ALL, "Shader definition find failed: %s\n", name);
		return DEFAULT_SHADER_DEF;
	}

	sh = GLSL_ParseDefinition(&shaderText, strippedName);
	if (!sh)
	{
		ri.Printf(PRINT_ALL, "Shader definition parsing failed: %s\n", name);
		return DEFAULT_SHADER_DEF;
	}
	else
	{
		sh->next        = hashTable[hash];
		hashTable[hash] = sh;
		return sh;
	}
}

void GLSL_LoadDefinitions(void)
{
	//FIXME: Also load from external files in the future...
	//For no just copy the existing data to our searchable string
	definitionText = (char *)Com_Allocate(strlen(defaultShaderDefinitions) * sizeof(char) + 1);
	Com_Memset(definitionText, '\0', strlen(defaultShaderDefinitions) + 1);
	Q_strncpyz(definitionText, defaultShaderDefinitions, strlen(defaultShaderDefinitions));
}

static void GLSL_PrintInfoLog(GLhandleARB object, qboolean developerOnly)
{
	char        *msg;
	static char msgPart[1024];
	int         maxLength = 0;
	int         i;
	int         printLevel = developerOnly ? PRINT_DEVELOPER : PRINT_ALL;

	qglGetObjectParameterivARB(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &maxLength);

	if (maxLength <= 0)
	{
		ri.Printf(printLevel, "No compile log.\n");
		return;
	}

	ri.Printf(printLevel, "compile log:\n");

	if (maxLength < 1023)
	{
		qglGetInfoLogARB(object, maxLength, &maxLength, msgPart);

		msgPart[maxLength + 1] = '\0';

		ri.Printf(printLevel, "%s\n", msgPart);
	}
	else
	{
		msg = ri.Hunk_AllocateTempMemory(maxLength);

		qglGetInfoLogARB(object, maxLength, &maxLength, msg);

		for (i = 0; i < maxLength; i += 1024)
		{
			Q_strncpyz(msgPart, msg + i, sizeof(msgPart));

			ri.Printf(printLevel, "%s\n", msgPart);
		}
		ri.Hunk_FreeTempMemory(msg);
	}
}

static void GLSL_PrintShaderSource(GLhandleARB object)
{
	char        *msg;
	static char msgPart[1024];
	int         maxLength = 0;
	int         i;

	qglGetObjectParameterivARB(object, GL_OBJECT_SHADER_SOURCE_LENGTH_ARB, &maxLength);

	msg = ri.Hunk_AllocateTempMemory(maxLength);

	qglGetShaderSourceARB(object, maxLength, &maxLength, msg);

	for (i = 0; i < maxLength; i += 1024)
	{
		Q_strncpyz(msgPart, msg + i, sizeof(msgPart));
		ri.Printf(PRINT_ALL, "%s\n", msgPart);
	}

	ri.Hunk_FreeTempMemory(msg);
}

qboolean GLSL_LoadShaderBinary(programInfo_t *info, size_t programNum)
{
#ifdef GLEW_ARB_get_program_binary
	GLint            success;
	GLint            fileLength;
	void             *binary;
	byte             *binaryptr;
	GLShaderHeader_t shaderHeader;
	shaderProgram_t  *shaderProgram;
	unsigned int     i;

	// we need to recompile the shaders
	if (r_recompileShaders->integer)
	{
		return qfalse;
	}

	// don't even try if the necessary functions aren't available
	if (!glConfig2.getProgramBinaryAvailable)
	{
		return qfalse;
	}

	fileLength = ri.FS_ReadFile(va("glsl/%s/%s_%u.bin", info->name, info->name, ( unsigned int ) programNum), &binary);

	// file empty or not found
	if (fileLength <= 0)
	{
		return qfalse;
	}

	binaryptr = ( byte * )binary;

	// get the shader header from the file
	memcpy(&shaderHeader, binaryptr, sizeof(shaderHeader));
	binaryptr += sizeof(shaderHeader);

	// check if this shader binary is the correct format
	if (shaderHeader.version != GL_SHADER_VERSION)
	{
		ri.FS_FreeFile(binary);
		return qfalse;
	}

	// make sure this shader uses the same number of macros
	if (shaderHeader.numMacros != info->numMacros)
	{
		ri.FS_FreeFile(binary);
		return qfalse;
	}

	// make sure this shader uses the same macros
	for (i = 0; i < shaderHeader.numMacros; i++)
	{
		if (info->macros[i] != shaderHeader.macros[i])
		{
			ri.FS_FreeFile(binary);
			return qfalse;
		}
	}

	// make sure the checksums for the source code match
	if (shaderHeader.checkSum != info->checkSum)
	{
		ri.FS_FreeFile(binary);
		return qfalse;
	}

	// load the shader
	shaderProgram          = &info->list->programs[programNum];
	shaderProgram->program = qglCreateProgramObjectARB();
	glProgramBinary(shaderProgram->program, shaderHeader.binaryFormat, binaryptr, shaderHeader.binaryLength);
	glGetProgramiv(shaderProgram->program, GL_LINK_STATUS, &success);

	if (!success)
	{
		ri.FS_FreeFile(binary);
		qglDeleteObjectARB(shaderProgram->program);
		return qfalse;
	}

	ri.FS_FreeFile(binary);
	return qtrue;
#else
	return qfalse;
#endif
}

void GLSL_SaveShaderBinary(programInfo_t *info, size_t programNum)
{
#ifdef GLEW_ARB_get_program_binary
	GLint            binaryLength;
	GLuint           binarySize = 0;
	byte             *binary;
	byte             *binaryptr;
	GLShaderHeader_t shaderHeader;
	shaderProgram_t  *shaderProgram;
	unsigned int     i;

	// don't even try if the necessary functions aren't available
	if (!glConfig2.getProgramBinaryAvailable)
	{
		return;
	}

	shaderProgram = &info->list->programs[programNum];

	memset(&shaderHeader, 0, sizeof(shaderHeader));

	// find output size
	binarySize += sizeof(shaderHeader);
	glGetProgramiv(shaderProgram->program, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
	binarySize += binaryLength;

	binaryptr = binary = ( byte * )ri.Hunk_AllocateTempMemory(binarySize);

	// reserve space for the header
	binaryptr += sizeof(shaderHeader);

	// get the program binary and write it to the buffer
	glGetProgramBinary(shaderProgram->program, binaryLength, NULL, &shaderHeader.binaryFormat, binaryptr);

	// set the header
	shaderHeader.version   = GL_SHADER_VERSION;
	shaderHeader.numMacros = info->numMacros;

	for (i = 0; i < shaderHeader.numMacros; i++)
	{
		shaderHeader.macros[i] = info->macros[i];
	}

	shaderHeader.binaryLength = binaryLength;
	shaderHeader.checkSum     = info->checkSum;

	// write the header to the buffer
	memcpy(( void * )binary, &shaderHeader, sizeof(shaderHeader));

	ri.FS_WriteFile(va("glsl/%s/%s_%u.bin", info->name, info->name, ( unsigned int ) programNum), binary, binarySize);

	ri.Hunk_FreeTempMemory(binary);
#endif
}

static qboolean GLSL_HasConflictingMacros(int compilemacro, int usedmacros)
{
	switch (compilemacro)
	{
	case USE_VERTEX_SKINNING:
		if (usedmacros & BIT(USE_VERTEX_ANIMATION))
		{
			return qtrue;
		}
		break;
	case USE_DEFORM_VERTEXES:
		if (glConfig.driverType != GLDRV_OPENGL3 || !r_vboDeformVertexes->integer)
		{
			return qtrue;
		}
		break;
	case USE_VERTEX_ANIMATION:
		if (usedmacros & BIT(USE_VERTEX_SKINNING))
		{
			return qtrue;
		}
		break;
	default:
		return qfalse;
	}

	return qfalse;
}

static qboolean GLSL_MissesRequiredMacros(int compilemacro, int usedmacros)
{
	switch (compilemacro)
	{
	case USE_PARALLAX_MAPPING:
		if (!(usedmacros & BIT(USE_NORMAL_MAPPING)))
		{
			return qtrue;
		}
		break;
	case USE_REFLECTIVE_SPECULAR:
		if (usedmacros & BIT(USE_NORMAL_MAPPING))
		{
			return qtrue;
		}
		break;
	case USE_VERTEX_SKINNING:
		if (!glConfig2.vboVertexSkinningAvailable)
		{
			return qtrue;
		}
		break;
	default:
		return qfalse;
	}
	return qfalse;
}

static unsigned int GLSL_GetRequiredVertexAttributes(int compilemacro)
{
	unsigned int attr = 0;

	switch (compilemacro)
	{
	case TWOSIDED:
		attr = ATTR_NORMAL;
		break;
	case USE_VERTEX_ANIMATION:
		attr = ATTR_NORMAL | ATTR_POSITION2 | ATTR_NORMAL2;

		if (r_normalMapping->integer)
		{
			attr |= ATTR_TANGENT2 | ATTR_BINORMAL2;
		}
		break;
	case USE_VERTEX_SKINNING:
		attr = ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;
		break;
	case USE_DEFORM_VERTEXES:
		attr = ATTR_NORMAL;
		break;
	case USE_NORMAL_MAPPING:
		attr = ATTR_NORMAL | ATTR_TANGENT | ATTR_BINORMAL;
		break;
	case USE_TCGEN_ENVIRONMENT:
		attr = ATTR_NORMAL;
		break;
	case USE_TCGEN_LIGHTMAP:
		attr = ATTR_LIGHTCOORD;
		break;
	default:
		attr = 0;
		break;
	}

	return attr;
}

static void GLSL_GetShaderExtraDefines(char **defines, int *size)
{
	static char bufferExtra[32000];

	float fbufWidthScale, fbufHeightScale;
	float npotWidthScale, npotHeightScale;

	Com_Memset(bufferExtra, 0, sizeof(bufferExtra));

#define BUFFEXT(...) Q_strcat(bufferExtra, sizeof(bufferExtra), va(__VA_ARGS__))
	
	/*
#define SIMPLEDEF(x, y) BUFFEXT("#ifndef " x "\n#define " y "\n#endif\n")
#define BUFFCVARI(x) BUFFEXT("#ifndef " #x "\n#define " #x " %f\n#endif\n", x->integer)
#define BUFFCVARF(x) BUFFEXT("#ifndef " #x "\n#define " #x " %f\n#endif\n", x->value)
#define IFCVARI(x) if(x->value) BUFFCVARI(x)
#define IFCVARF(x) if(x->value) BUFFCVARF(x)
#define ENABLEIFCVAR(x) if(x->value) BUFFEXT("#ifndef " #x "\n#define " #x " 1\n#endif\n")
#define SIMPLEDEFV(x,y) BUFFEXT("#ifndef " x "\n#define " x " " y "\n#endif\n")
	BUFFCVARF(r_specularExponent);
	BUFFCVARF(r_specularExponent2);
	BUFFCVARF(r_specularScale);
	BUFFCVARF(r_normalScale);
	SIMPLEDEFV("M_PI", "3.14159265358979323846f");
	*/


	// HACK: add some macros to avoid extra uniforms and save speed and code maintenance
	BUFFEXT("#ifndef r_SpecularExponent\n#define r_SpecularExponent %f\n#endif\n", r_specularExponent->value);
	BUFFEXT("#ifndef r_SpecularExponent2\n#define r_SpecularExponent2 %f\n#endif\n", r_specularExponent2->value);
	BUFFEXT("#ifndef r_SpecularScale\n#define r_SpecularScale %f\n#endif\n", r_specularScale->value);
	//BUFFEXT("#ifndef r_NormalScale\n#define r_NormalScale %f\n#endif\n", r_normalScale->value);
	
	BUFFEXT("#ifndef M_PI\n#define M_PI 3.14159265358979323846f\n#endif\n");
	BUFFEXT("#ifndef MAX_SHADOWMAPS\n#define MAX_SHADOWMAPS %i\n#endif\n", MAX_SHADOWMAPS);
	BUFFEXT("#ifndef MAX_SHADER_DEFORM_PARMS\n#define MAX_SHADER_DEFORM_PARMS %i\n#endif\n", MAX_SHADER_DEFORM_PARMS);
	BUFFEXT("#ifndef deform_t\n"
	            "#define deform_t\n"
	            "#define DEFORM_WAVE %i\n"
	            "#define DEFORM_BULGE %i\n"
	            "#define DEFORM_MOVE %i\n"
	            "#endif\n",
	            DEFORM_WAVE,
	            DEFORM_BULGE,
	            DEFORM_MOVE);

	BUFFEXT("#ifndef genFunc_t\n"
	            "#define genFunc_t\n"
	            "#define GF_NONE %1.1f\n"
	            "#define GF_SIN %1.1f\n"
	            "#define GF_SQUARE %1.1f\n"
	            "#define GF_TRIANGLE %1.1f\n"
	            "#define GF_SAWTOOTH %1.1f\n"
	            "#define GF_INVERSE_SAWTOOTH %1.1f\n"
	            "#define GF_NOISE %1.1f\n"
	            "#endif\n",
	            ( float ) GF_NONE,
	            ( float ) GF_SIN,
	            ( float ) GF_SQUARE,
	            ( float ) GF_TRIANGLE,
	            ( float ) GF_SAWTOOTH,
	            ( float ) GF_INVERSE_SAWTOOTH,
	            ( float ) GF_NOISE);

	/*
	 BUFFEXT("#ifndef deformGen_t\n"
	 "#define deformGen_t\n"
	 "#define DGEN_WAVE_SIN %1.1f\n"
	 "#define DGEN_WAVE_SQUARE %1.1f\n"
	 "#define DGEN_WAVE_TRIANGLE %1.1f\n"
	 "#define DGEN_WAVE_SAWTOOTH %1.1f\n"
	 "#define DGEN_WAVE_INVERSE_SAWTOOTH %1.1f\n"
	 "#define DGEN_BULGE %i\n"
	 "#define DGEN_MOVE %i\n"
	 "#endif\n",
	 (float)DGEN_WAVE_SIN,
	 (float)DGEN_WAVE_SQUARE,
	 (float)DGEN_WAVE_TRIANGLE,
	 (float)DGEN_WAVE_SAWTOOTH,
	 (float)DGEN_WAVE_INVERSE_SAWTOOTH,
	 DGEN_BULGE,
	 DGEN_MOVE);
	*/

	/*
	 BUFFEXT("#ifndef colorGen_t\n"
	 "#define colorGen_t\n"
	 "#define CGEN_VERTEX %i\n"
	 "#define CGEN_ONE_MINUS_VERTEX %i\n"
	 "#endif\n",
	 CGEN_VERTEX,
	 CGEN_ONE_MINUS_VERTEX);

	 BUFFEXT("#ifndef alphaGen_t\n"
	 "#define alphaGen_t\n"
	 "#define AGEN_VERTEX %i\n"
	 "#define AGEN_ONE_MINUS_VERTEX %i\n"
	 "#endif\n",
	 AGEN_VERTEX,
	 AGEN_ONE_MINUS_VERTEX);
	 */

	BUFFEXT("#ifndef alphaTest_t\n"
	            "#define alphaTest_t\n"
	            "#define ATEST_GT_0 %i\n"
	            "#define ATEST_LT_128 %i\n"
	            "#define ATEST_GE_128 %i\n"
	            "#endif\n",
	            ATEST_GT_0,
	            ATEST_LT_128,
	            ATEST_GE_128);

	fbufWidthScale  = Q_recip(( float ) glConfig.vidWidth);
	fbufHeightScale = Q_recip(( float ) glConfig.vidHeight);
	BUFFEXT("#ifndef r_FBufScale\n#define r_FBufScale vec2(%f, %f)\n#endif\n", fbufWidthScale, fbufHeightScale);

	if (glConfig2.textureNPOTAvailable)
	{
		npotWidthScale  = 1;
		npotHeightScale = 1;
	}
	else
	{
		npotWidthScale  = ( float ) glConfig.vidWidth / ( float ) NearestPowerOfTwo(glConfig.vidWidth);
		npotHeightScale = ( float ) glConfig.vidHeight / ( float ) NearestPowerOfTwo(glConfig.vidHeight);
	}

	BUFFEXT("#ifndef r_NPOTScale\n#define r_NPOTScale vec2(%f, %f)\n#endif\n", npotWidthScale, npotHeightScale);

	if (glConfig.driverType == GLDRV_MESA)
	{
		BUFFEXT("#ifndef GLDRV_MESA\n#define GLDRV_MESA 1\n#endif\n");
	}

	if (glConfig.hardwareType == GLHW_ATI)
	{
		BUFFEXT("#ifndef GLHW_ATI\n#define GLHW_ATI 1\n#endif\n");
	}
	else if (glConfig.hardwareType == GLHW_ATI_DX10)
	{
		BUFFEXT("#ifndef GLHW_ATI_DX10\n#define GLHW_ATI_DX10 1\n#endif\n");
	}
	else if (glConfig.hardwareType == GLHW_NV_DX10)
	{
		BUFFEXT("#ifndef GLHW_NV_DX10\n#define GLHW_NV_DX10 1\n#endif\n");
	}

	if (r_shadows->integer >= SHADOWING_ESM16 && glConfig2.textureFloatAvailable && glConfig2.framebufferObjectAvailable)
	{
		if (r_shadows->integer == SHADOWING_ESM16 || r_shadows->integer == SHADOWING_ESM32)
		{
			BUFFEXT("#ifndef ESM\n#define ESM 1\n#endif\n");
		}
		else if (r_shadows->integer == SHADOWING_EVSM32)
		{
			BUFFEXT("#ifndef EVSM\n#define EVSM 1\n#endif\n");

			// The exponents for the EVSM techniques should be less than ln(FLT_MAX/FILTER_SIZE)/2 {ln(FLT_MAX/1)/2 ~44.3}
			//         42.9 is the maximum possible value for FILTER_SIZE=15
			//         42.0 is the truncated value that we pass into the sample
			BUFFEXT("#ifndef r_EVSMExponents\n#define r_EVSMExponents vec2(%f, %f)\n#endif\n", 42.0f, 42.0f);

			if (r_evsmPostProcess->integer)
			{
				BUFFEXT("#ifndef r_EVSMPostProcess\n#define r_EVSMPostProcess 1\n#endif\n");
			}
		}
		else
		{
			BUFFEXT("#ifndef VSM\n#define VSM 1\n#endif\n");

			if (glConfig.hardwareType == GLHW_ATI)
			{
				BUFFEXT("#ifndef VSM_CLAMP\n#define VSM_CLAMP 1\n#endif\n");
			}
		}

		if ((glConfig.hardwareType == GLHW_NV_DX10 || glConfig.hardwareType == GLHW_ATI_DX10) && r_shadows->integer == SHADOWING_VSM32)
		{
			BUFFEXT("#ifndef VSM_EPSILON\n#define VSM_EPSILON 0.000001\n#endif\n");
		}
		else
		{
			BUFFEXT("#ifndef VSM_EPSILON\n#define VSM_EPSILON 0.0001\n#endif\n");
		}

		if (r_lightBleedReduction->value)
		{
			BUFFEXT("#ifndef r_LightBleedReduction\n#define r_LightBleedReduction %f\n#endif\n", r_lightBleedReduction->value);
		}

		if (r_overDarkeningFactor->value)
		{
			BUFFEXT("#ifndef r_OverDarkeningFactor\n#define r_OverDarkeningFactor %f\n#endif\n", r_overDarkeningFactor->value);
		}

		if (r_shadowMapDepthScale->value)
		{
			BUFFEXT("#ifndef r_ShadowMapDepthScale\n#define r_ShadowMapDepthScale %f\n#endif\n", r_shadowMapDepthScale->value);
		}

		if (r_debugShadowMaps->integer)
		{
			BUFFEXT("#ifndef r_DebugShadowMaps\n#define r_DebugShadowMaps %i\n#endif\n", r_debugShadowMaps->integer);
		}

		if (r_softShadows->integer == 1)
		{
			BUFFEXT("#ifndef PCSS\n#define PCSS 1\n#endif\n");
		}
		else if (r_softShadows->integer)
		{
			BUFFEXT("#ifndef r_PCFSamples\n#define r_PCFSamples %1.1f\n#endif\n", r_softShadows->value + 1.0f);
		}

		if (r_parallelShadowSplits->integer)
		{
			BUFFEXT("#ifndef r_ParallelShadowSplits_%i\n#define r_ParallelShadowSplits_%i\n#endif\n", r_parallelShadowSplits->integer, r_parallelShadowSplits->integer);
		}

		if (r_showParallelShadowSplits->integer)
		{
			BUFFEXT("#ifndef r_ShowParallelShadowSplits\n#define r_ShowParallelShadowSplits 1\n#endif\n");
		}
	}

	if (r_deferredShading->integer && glConfig2.maxColorAttachments >= 4 && glConfig2.textureFloatAvailable &&
	    glConfig2.drawBuffersAvailable && glConfig2.maxDrawBuffers >= 4)
	{
		if (r_deferredShading->integer == DS_STANDARD)
		{
			BUFFEXT("#ifndef r_DeferredShading\n#define r_DeferredShading 1\n#endif\n");
		}
	}

	if (r_hdrRendering->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable)
	{
		BUFFEXT("#ifndef r_HDRRendering\n#define r_HDRRendering 1\n#endif\n");
		BUFFEXT("#ifndef r_HDRContrastThreshold\n#define r_HDRContrastThreshold %f\n#endif\n", r_hdrContrastThreshold->value);
		BUFFEXT("#ifndef r_HDRContrastOffset\n#define r_HDRContrastOffset %f\n#endif\n", r_hdrContrastOffset->value);
		BUFFEXT("#ifndef r_HDRToneMappingOperator\n#define r_HDRToneMappingOperator_%i\n#endif\n", r_hdrToneMappingOperator->integer);
		BUFFEXT("#ifndef r_HDRGamma\n#define r_HDRGamma %f\n#endif\n", r_hdrGamma->value);
	}

	if (r_precomputedLighting->integer)
	{
		BUFFEXT("#ifndef r_precomputedLighting\n#define r_precomputedLighting 1\n#endif\n");
	}

	if (r_heatHazeFix->integer && glConfig2.framebufferBlitAvailable && /*glConfig.hardwareType != GLHW_ATI && glConfig.hardwareType != GLHW_ATI_DX10 &&*/ glConfig.driverType != GLDRV_MESA)
	{
		BUFFEXT("#ifndef r_heatHazeFix\n#define r_heatHazeFix 1\n#endif\n");
	}

	if (glConfig2.vboVertexSkinningAvailable)
	{
		BUFFEXT("#ifndef r_VertexSkinning\n#define r_VertexSkinning 1\n#endif\n");
		BUFFEXT("#ifndef MAX_GLSL_BONES\n#define MAX_GLSL_BONES %i\n#endif\n", glConfig2.maxVertexSkinningBones);
	}
	else
	{
		BUFFEXT("#ifndef MAX_GLSL_BONES\n#define MAX_GLSL_BONES %i\n#endif\n", 4);
	}

	if (r_normalMapping->integer)
	{
		BUFFEXT("#ifndef r_NormalMapping\n#define r_NormalMapping 1\n#endif\n");
	}

	if (/* TODO: check for shader model 3 hardware  && */ r_normalMapping->integer && r_parallaxMapping->integer)
	{
		BUFFEXT("#ifndef r_ParallaxMapping\n#define r_ParallaxMapping 1\n#endif\n");
	}

	if (r_wrapAroundLighting->value)
	{
		BUFFEXT("#ifndef r_WrapAroundLighting\n#define r_WrapAroundLighting %f\n#endif\n", r_wrapAroundLighting->value);
	}

	if (r_halfLambertLighting->integer)
	{
		BUFFEXT("#ifndef r_HalfLambertLighting\n#define r_HalfLambertLighting 1\n#endif\n");
	}

	if (r_rimLighting->integer)
	{
		BUFFEXT("#ifndef r_RimLighting\n#define r_RimLighting 1\n#endif\n");
		BUFFEXT("#ifndef r_RimColor\n#define r_RimColor vec4(0.26, 0.19, 0.16, 0.0)\n#endif\n");
		BUFFEXT("#ifndef r_RimExponent\n#define r_RimExponent %f\n#endif\n", r_rimExponent->value);
	}

	// OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
	// so we have to reset the line counting
	BUFFEXT("#line 0\n");

	*size    = strlen(bufferExtra) + 1;
	*defines = (char *) Com_Allocate(*size);
	Com_Memset(*defines, 0, *size);
	Q_strcat(*defines, *size, bufferExtra);
}

static void GLSL_GetShaderHeader(GLenum shaderType, char *dest, int size)
{
	dest[0] = '\0';

	// HACK: abuse the GLSL preprocessor to turn GLSL 1.20 shaders into 1.30 ones
	if (glConfig2.glslMajorVersion > 1 || (glConfig2.glslMajorVersion == 1 && glConfig2.glslMinorVersion >= 30))
	{
		Q_strcat(dest, size, "#version 130\n");

		if (shaderType == GL_VERTEX_SHADER)
		{
			Q_strcat(dest, size, "#define attribute in\n");
			Q_strcat(dest, size, "#define varying out\n");
		}
		else
		{
			Q_strcat(dest, size, "#define varying in\n");

			Q_strcat(dest, size, "out vec4 out_Color;\n");
			Q_strcat(dest, size, "#define gl_FragColor out_Color\n");
		}

		Q_strcat(dest, size, "#define textureCube texture\n");
	}
	else
	{
		Q_strcat(dest, size, "#version 120\n");
	}
}

static int GLSL_CompileGPUShader(GLhandleARB program, GLhandleARB *prevShader, const GLcharARB *buffer, int size, GLenum shaderType, const char *name)
{
	GLint       compiled;
	GLhandleARB shader;

	shader = qglCreateShaderObjectARB(shaderType);

	qglShaderSourceARB(shader, 1, (const GLcharARB **)&buffer, &size);

	// compile shader
	qglCompileShaderARB(shader);

	// check if shader compiled
	qglGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
	if (!compiled)
	{
		ri.FS_WriteFile(va("debug/%s_%s.debug", name, (shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")), buffer, size);
		GLSL_PrintShaderSource(shader);
		GLSL_PrintInfoLog(shader, qfalse);
		ri.Error(ERR_FATAL, "Couldn't compile shader");
		return 0;
	}

	//GLSL_PrintInfoLog(shader, qtrue);
	//GLSL_PrintShaderSource(shader);

	if (*prevShader)
	{
		qglDetachObjectARB(program, *prevShader);
		qglDeleteObjectARB(*prevShader);
	}

	// attach shader to program
	qglAttachObjectARB(program, shader);

	*prevShader = shader;

	return 1;
}

static void GLSL_GetShaderText(const char *name, GLenum shaderType, char **data, int *size, qboolean append)
{
	char fullname[MAX_QPATH];
	int  dataSize = 0;
	char *dataBuffer;

	if (shaderType == GL_VERTEX_SHADER)
	{
		Com_sprintf(fullname, sizeof(fullname), "%s_vp", name);
		ri.Printf(PRINT_ALL, "...loading vertex shader '%s'\n", fullname);
	}
	else
	{
		Com_sprintf(fullname, sizeof(fullname), "%s_fp", name);
		ri.Printf(PRINT_ALL, "...loading vertex shader '%s'\n", fullname);
	}

	if (ri.FS_FOpenFileRead(va("glsl/%s.glsl", fullname), NULL, qfalse))
	{
		dataSize = ri.FS_ReadFile(va("glsl/%s.glsl", fullname), ( void ** ) &dataBuffer);
	}
	else
	{
		dataBuffer = NULL;
	}

	if (!dataBuffer)
	{
		const char *temp = NULL;

		temp = GetFallbackShader(fullname);
		if (temp)
		{
			//Found a fallback shader and will use it
			int strl = 0;
			strl = strlen(temp) + 1;
			if (append && *size)
			{
				*data = ( char * ) realloc(*data, *size + strl);
				Com_Memset(*data + *size, 0, strl);

			}
			else
			{
				*data = (char *) Com_Allocate(strl);
				Com_Memset(*data, 0, strl);
			}

			*size += strl;

			Q_strcat(*data, *size, temp);
			Q_strcat(*data, *size, "\n");
		}
		else
		{
			ri.Error(ERR_FATAL, "Couldn't load shader %s", fullname);
		}
	}
	else
	{
		++dataSize; //We incease this for the newline
		if (append && *size)
		{
			*data = ( char * ) realloc(*data, *size + dataSize);
			Com_Memset(*data + *size, 0, dataSize);
		}
		else
		{
			*data = (char *) Com_Allocate(dataSize);
			Com_Memset(*data, 0, dataSize);
		}

		*size += dataSize;

		Q_strcat(*data, *size, dataBuffer);
		Q_strcat(*data, *size, "\n");
	}

	if (dataBuffer)
	{
		ri.FS_FreeFile(dataBuffer);
	}

	Com_Printf("Loaded shader '%s'\n", fullname);
}

static char *GLSL_BuildGPUShaderText(const char *mainShaderName, const char *libShaderNames, GLenum shaderType)
{
	GLchar *mainBuffer = NULL;
	int    mainSize    = 0;
	char   *token;

	int  libsSize    = 0;
	char *libsBuffer = NULL;        // all libs concatenated

	char **libs = ( char ** ) &libShaderNames;

	char *shaderText = NULL;

	char *shaderExtra = NULL;
	int  extraSize = 0;

	char *bufferFinal = NULL;
	int  sizeFinal;

	GL_CheckErrors();

	while (1)
	{
		token = COM_ParseExt2(libs, qfalse);

		if (!token[0])
		{
			break;
		}
		GLSL_GetShaderText(token, shaderType, &libsBuffer, &libsSize, qtrue);
	}

	// load main() program
	GLSL_GetShaderText(mainShaderName, shaderType, &mainBuffer, &mainSize, qfalse);

	if (!libsBuffer && !mainBuffer)
	{
		ri.Error(ERR_FATAL, "Shader loading failed!\n");
	}
	
	GLSL_GetShaderExtraDefines(&shaderExtra, &extraSize);

	sizeFinal = extraSize + mainSize + libsSize;

	bufferFinal = (char *)ri.Hunk_AllocateTempMemory(sizeFinal);

	strcpy(bufferFinal, shaderExtra);

	if (libsSize > 0)
	{
		Q_strcat(bufferFinal, sizeFinal, libsBuffer);
	}

	Q_strcat(bufferFinal, sizeFinal, mainBuffer);

	shaderText = Com_Allocate(sizeFinal);
	strcpy(shaderText, bufferFinal);
	ri.Hunk_FreeTempMemory(bufferFinal);
	Com_Dealloc(shaderExtra);

	Com_Dealloc(mainBuffer);
	Com_Dealloc(libsBuffer);

	return shaderText;
}

/*
This whole method is stupid, clean this shit up
*/
static qboolean GLSL_GenerateMacroString(shaderProgramList_t *program, const char *macros, int permutation, char **out)
{
	int i;
	int macroatrib = 0;

	*out = (char *) Com_Allocate(1000);
	Com_Memset(*out, 0, 1000);

	if (permutation)
	{
		for (i = 0; i < MAX_MACROS; i++)
		{
			if (program->macromap[i] != -1 && (permutation & BIT(program->macromap[i])))
			{
				macroatrib |= BIT(i);
			}
		}

		for (i = 0; i < MAX_MACROS; i++)
		{
			if (macroatrib & BIT(i))
			{
				if (GLSL_HasConflictingMacros(i, macroatrib))
				{
					return qfalse;
				}

				if (GLSL_MissesRequiredMacros(i, macroatrib))
				{
					return qfalse;
				}

				Q_strcat(*out, 1000, va("%s ", complieMacroNames[i]));
			}
		}
	}

	if (macros)
	{
		Q_strcat(*out, 1000, macros);
	}

	return qtrue;
}

static void GLSL_LinkProgram(GLhandleARB program)
{
	GLint linked;

#ifdef GLEW_ARB_get_program_binary
	// Apparently, this is necessary to get the binary program via glGetProgramBinary
	if (glConfig2.getProgramBinaryAvailable)
	{
		glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
	}
#endif

	qglLinkProgramARB(program);

	qglGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &linked);
	if (!linked)
	{
		GLSL_PrintInfoLog(program, qfalse);
		ri.Printf(PRINT_ALL, "\n");
		ri.Error(ERR_DROP, "shaders failed to link");
	}
}

static void GLSL_ValidateProgram(GLhandleARB program)
{
	GLint validated;

	qglValidateProgramARB(program);

	qglGetObjectParameterivARB(program, GL_OBJECT_VALIDATE_STATUS_ARB, &validated);
	if (!validated)
	{
		GLSL_PrintInfoLog(program, qfalse);
		ri.Printf(PRINT_ALL, "\n");
		ri.Error(ERR_DROP, "shaders failed to validate");
	}
}

static void GLSL_ShowProgramUniforms(GLhandleARB program)
{
	int    i, count, size;
	GLenum type;
	char   uniformName[1000];

	// install the executables in the program object as part of current state.
	qglUseProgramObjectARB(program);

	// check for GL Errors

	// query the number of active uniforms
	qglGetObjectParameterivARB(program, GL_OBJECT_ACTIVE_UNIFORMS_ARB, &count);

	// Loop over each of the active uniforms, and set their value
	for (i = 0; i < count; i++)
	{
		qglGetActiveUniformARB(program, i, sizeof(uniformName), NULL, &size, &type, uniformName);

		Ren_LogComment("active uniform: '%s'\n", uniformName)
	}

	qglUseProgramObjectARB(0);
}

void GLSL_InitUniforms(shaderProgram_t *program)
{
	int i, size;

	GLint *uniforms = program->uniforms;

	size = 0;

	for (i = 0; i < UNIFORM_COUNT; i++)
	{
		uniforms[i] = qglGetUniformLocationARB(program->program, uniformsInfo[i].name);

		if (uniforms[i] == -1)
		{
			continue;
		}

		program->uniformBufferOffsets[i] = size;
		switch (uniformsInfo[i].type)
		{
		case GLSL_BOOL:
			size += sizeof(GLboolean);
			break;
		case GLSL_INT:
			size += sizeof(GLint);
			break;
		case GLSL_FLOAT:
			size += sizeof(GLfloat);
			break;
		case GLSL_FLOAT5:
			size += sizeof(vec_t) * 5;
			break;
		case GLSL_VEC2:
			size += sizeof(vec_t) * 2;
			break;
		case GLSL_VEC3:
			size += sizeof(vec_t) * 3;
			break;
		case GLSL_VEC4:
			size += sizeof(vec_t) * 4;
			break;
		case GLSL_MAT16:
			size += sizeof(vec_t) * 16;
			break;
		default:
			break;
		}
	}

	program->uniformBuffer = (char *)Com_Allocate(size);
}

void GLSL_FinishGPUShader(shaderProgram_t *program)
{
	GLSL_ValidateProgram(program->program);
	GLSL_ShowProgramUniforms(program->program);
	GL_CheckErrors();
}

void GLSL_SetUniformBoolean(shaderProgram_t *program, int uniformNum, GLboolean value)
{
	GLint     *uniforms = program->uniforms;
	GLboolean *compare  = (GLboolean *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);


	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_BOOL)
	{
		ri.Error(ERR_FATAL, "GLSL_SetUniformBoolean: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (value == *compare)
	{
		return;
	}

	*compare = value;

	qglUniform1iARB(uniforms[uniformNum], value);
}

void GLSL_SetUniformInt(shaderProgram_t *program, int uniformNum, GLint value)
{
	GLint *uniforms = program->uniforms;
	GLint *compare  = (GLint *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);


	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_INT)
	{
		ri.Error(ERR_FATAL, "GLSL_SetUniformInt: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (value == *compare)
	{
		return;
	}

	*compare = value;

	qglUniform1iARB(uniforms[uniformNum], value);
}

void GLSL_SetUniformFloat(shaderProgram_t *program, int uniformNum, GLfloat value)
{
	GLint   *uniforms = program->uniforms;
	GLfloat *compare  = (GLfloat *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_FLOAT)
	{
		ri.Error(ERR_FATAL, "GLSL_SetUniformFloat: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (value == *compare)
	{
		return;
	}

	*compare = value;

	qglUniform1fARB(uniforms[uniformNum], value);
}

void GLSL_SetUniformVec2(shaderProgram_t *program, int uniformNum, const vec2_t v)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare  = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_VEC2)
	{
		ri.Error(ERR_FATAL, "GLSL_SetUniformVec2: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (v[0] == compare[0] && v[1] == compare[1])
	{
		return;
	}

	compare[0] = v[0];
	compare[1] = v[1];

	qglUniform2fARB(uniforms[uniformNum], v[0], v[1]);
}

void GLSL_SetUniformVec3(shaderProgram_t *program, int uniformNum, const vec3_t v)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare  = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_VEC3)
	{
		ri.Error(ERR_FATAL, "GLSL_SetUniformVec3: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (VectorCompare(v, compare))
	{
		return;
	}

	VectorCopy(v, compare);

	qglUniform3fARB(uniforms[uniformNum], v[0], v[1], v[2]);
}

void GLSL_SetUniformVec4(shaderProgram_t *program, int uniformNum, const vec4_t v)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare  = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_VEC4)
	{
		ri.Error(ERR_FATAL, "GLSL_SetUniformVec4: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (Vector4Compare(v, compare))
	{
		return;
	}

	Vector4Copy(v, compare);

	qglUniform4fARB(uniforms[uniformNum], v[0], v[1], v[2], v[3]);
}

void GLSL_SetUniformFloat5(shaderProgram_t *program, int uniformNum, const vec5_t v)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare  = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_FLOAT5)
	{
		ri.Error(ERR_FATAL, "GLSL_SetUniformFloat5: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (Vector5Compare(v, compare))
	{
		return;
	}

	Vector5Copy(v, compare);

	qglUniform1fvARB(uniforms[uniformNum], 5, v);
}

void GLSL_SetUniformMatrix16(shaderProgram_t *program, int uniformNum, const matrix_t matrix)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare  = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_MAT16)
	{
		ri.Error(ERR_FATAL, "GLSL_SetUniformMatrix16: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (MatrixCompare(matrix, compare))
	{
		return;
	}

	MatrixCopy(matrix, compare);

	qglUniformMatrix4fvARB(uniforms[uniformNum], 1, GL_FALSE, matrix);
}

void GLSL_SetUniformFloatARR(shaderProgram_t *program, int uniformNum, float *floatarray, int arraysize)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_FLOATARR)
	{
		ri.Error(ERR_FATAL, "GLSL_SetUniformFloatARR: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	glUniform1fv(uniforms[uniformNum], arraysize, floatarray);
}

void GLSL_SetUniformVec4ARR(shaderProgram_t *program, int uniformNum, vec4_t *vectorarray, int arraysize)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_VEC4ARR)
	{
		ri.Error(ERR_FATAL, "GLSL_SetUniformVec4ARR: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	glUniform4fv(uniforms[uniformNum], arraysize, &vectorarray[0][0]);
}

void GLSL_SetUniformMatrix16ARR(shaderProgram_t *program, int uniformNum, matrix_t *matrixarray, int arraysize)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_MAT16ARR)
	{
		ri.Error(ERR_FATAL, "GLSL_SetUniformMatrix16ARR: wrong type for uniform %s in program %s\n", uniformsInfo[uniformNum].name, program->name);
		return;
	}

	glUniformMatrix4fv(uniforms[uniformNum], arraysize, GL_FALSE, &matrixarray[0][0]);

}

static void GLSL_BindAttribLocations(GLuint program)
{
	glBindAttribLocation(program, ATTR_INDEX_POSITION, "attr_Position");
	glBindAttribLocation(program, ATTR_INDEX_TEXCOORD0, "attr_TexCoord0");
	glBindAttribLocation(program, ATTR_INDEX_TEXCOORD1, "attr_TexCoord1");

	//glBindAttribLocation(program, ATTR_INDEX_TEXCOORD2, "attr_TexCoord2");
	//glBindAttribLocation(program, ATTR_INDEX_TEXCOORD3, "attr_TexCoord3");

	glBindAttribLocation(program, ATTR_INDEX_TANGENT, "attr_Tangent");
	glBindAttribLocation(program, ATTR_INDEX_BINORMAL, "attr_Binormal");
	glBindAttribLocation(program, ATTR_INDEX_NORMAL, "attr_Normal");
	glBindAttribLocation(program, ATTR_INDEX_COLOR, "attr_Color");
	glBindAttribLocation(program, ATTR_INDEX_BONE_INDEXES, "attr_BoneIndexes");
	glBindAttribLocation(program, ATTR_INDEX_BONE_WEIGHTS, "attr_BoneWeights");
	glBindAttribLocation(program, ATTR_INDEX_POSITION2, "attr_Position2");
	glBindAttribLocation(program, ATTR_INDEX_TANGENT2, "attr_Tangent2");
	glBindAttribLocation(program, ATTR_INDEX_BINORMAL2, "attr_Binormal2");
	glBindAttribLocation(program, ATTR_INDEX_NORMAL2, "attr_Normal2");
}

static qboolean GLSL_InitGPUShader2(shaderProgram_t *program, const char *name, const char *vpCode, const char *fpCode)
{
	Ren_LogComment("------- GLSL_InitGPUShader2 -------\n");

	if (strlen(name) >= MAX_QPATH)
	{
		ri.Error(ERR_DROP, "GLSL_InitGPUShader2: \"%s\" is too long", name);
	}

	Q_strncpyz(program->name, name, sizeof(program->name));

	program->program = qglCreateProgramObjectARB();

	if (!(GLSL_CompileGPUShader(program->program, &program->vertexShader, vpCode, strlen(vpCode), GL_VERTEX_SHADER_ARB, name)))
	{
		ri.Printf(PRINT_ALL, "GLSL_InitGPUShader2: Unable to load \"%s\" as GL_VERTEX_SHADER_ARB\n", name);
		qglDeleteObjectARB(program->program);
		return qfalse;
	}

	if (fpCode)
	{
		if (!(GLSL_CompileGPUShader(program->program, &program->fragmentShader, fpCode, strlen(fpCode), GL_FRAGMENT_SHADER_ARB, name)))
		{
			ri.Printf(PRINT_ALL, "GLSL_InitGPUShader2: Unable to load \"%s\" as GL_FRAGMENT_SHADER_ARB\n", name);
			qglDeleteObjectARB(program->program);
			return qfalse;
		}
	}

	GLSL_BindAttribLocations(program->program);

	GLSL_LinkProgram(program->program);

	return qtrue;
}

static qboolean GLSL_FinnishShaderTextAndCompile(programInfo_t *info, int permutation, const char *vertex, const char *frag, const char *macrostring)
{
	char vpSource[64000];
	char fpSource[64000];
	int  size = sizeof(vpSource);

	GLSL_GetShaderHeader(GL_VERTEX_SHADER, vpSource, size);
	GLSL_GetShaderHeader(GL_FRAGMENT_SHADER, fpSource, size);

	if (macrostring)
	{
		char **compileMacrosP = ( char ** ) &macrostring;
		char *token;

		while (1)
		{
			token = COM_ParseExt2(compileMacrosP, qfalse);

			if (!token[0])
			{
				break;
			}

			Q_strcat(vpSource, size, va("#ifndef %s\n#define %s 1\n#endif\n", token, token));
			Q_strcat(fpSource, size, va("#ifndef %s\n#define %s 1\n#endif\n", token, token));
		}
	}

	Q_strcat(vpSource, size, vertex);
	Q_strcat(fpSource, size, frag);

	if (GLSL_InitGPUShader2(&info->list->programs[permutation], info->name, vpSource, fpSource))
	{
		GLSL_SaveShaderBinary(info, permutation);
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

static qboolean GLSL_GetProgramPermutation(programInfo_t *info, int permutation, const char *vertex, const char *frag, const char *macrostring)
{
	//load bin
	if (GLSL_LoadShaderBinary(info, permutation) || GLSL_FinnishShaderTextAndCompile(info, permutation, vertex, frag, macrostring))
	{
		GLSL_InitUniforms(&info->list->programs[permutation]);
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

static void GLSL_SetInitialUniformValues(programInfo_t *info, int permutation)
{
	int i, location;
	GLSL_BindProgram(&info->list->programs[permutation]);

	for (i = 0; i < info->numUniformValues; i++)
	{
		location = qglGetUniformLocationARB(info->list->programs[permutation].program, info->uniformValues[i].type.name);

		if (location == -1)
		{
			Ren_LogComment("Cannot find uniform \"%s\" from program: %s %d\n", info->uniformValues[i].type.name, info->name, location);
		}
		else
		{
			Ren_LogComment("Setting initial uniform \"%s\" value: %d\n", info->uniformValues[i].type.name, *((int *)info->uniformValues[i].value));
		}

		switch (info->uniformValues[i].type.type)
		{
		case GLSL_INT:
			//GLSL_SetUniformInt(&info->list->programs[permutation],location,*((int *)info->uniformValues[i].value));
			glUniform1i(location, *((int *)info->uniformValues[i].value));
			break;
		default:
			ri.Error(ERR_FATAL, "Only INT supported atm");
			break;
		}
	}

	GLSL_BindNullProgram();
}

void GLSL_GenerateCheckSum(programInfo_t *info, const char *vertex, const char *fragment)
{
	char *fullSource;
	int  size = 0;

	size += strlen(vertex);
	size += strlen(fragment);
	size *= sizeof(char);
	size += 1;

	fullSource = (char *)ri.Hunk_AllocateTempMemory(size);

	Q_strcat(fullSource, size, vertex);
	Q_strcat(fullSource, size, fragment);

	info->checkSum = Com_BlockChecksum(fullSource, strlen(fullSource));

	ri.Hunk_FreeTempMemory(fullSource);
}

qboolean GLSL_CompileShaderProgram(programInfo_t *info)
{
	char   *vertexShader   = GLSL_BuildGPUShaderText(info->filename, info->vertexLibraries, GL_VERTEX_SHADER);
	char   *fragmentShader = GLSL_BuildGPUShaderText((info->fragFilename ? info->fragFilename : info->filename), info->fragmentLibraries, GL_FRAGMENT_SHADER);
	int    startTime, endTime;
	size_t numPermutations = 0, numCompiled = 0, tics = 0, nextTicCount = 0;
	int    i               = 0, x = 0;

	GLSL_GenerateCheckSum(info, vertexShader, fragmentShader);

	info->list = (shaderProgramList_t *)Com_Allocate(sizeof(shaderProgramList_t));
	Com_Memset(info->list, 0, sizeof(shaderProgramList_t));

	if (info->numMacros > 0)
	{
		for (i = 0; i < MAX_MACROS; i++)
		{
			info->list->macromap[i] = -1;
			for (x = 0; x < info->numMacros; x++)
			{
				if (i == info->macros[x])
				{
					info->list->macromap[i] = x;
				}
			}
		}
		info->list->mappedMacros = i;
	}
	else
	{
		for (i = 0; i < MAX_MACROS; i++)
		{
			info->list->macromap[i] = -1;
		}
		info->list->mappedMacros = 0;
	}

	numPermutations = BIT(info->numMacros);

	info->list->permutations = numPermutations;

	ri.Printf(PRINT_ALL, "...compiling %s shaders\n", info->name);
	ri.Printf(PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n");
	ri.Printf(PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n");

	startTime = ri.Milliseconds();

	info->list->programs = (shaderProgram_t *)Com_Allocate(sizeof(shaderProgram_t) * numPermutations);
	Com_Memset(info->list->programs, 0, sizeof(shaderProgram_t) * numPermutations);

	for (i = 0; i < numPermutations; i++)
	{
		char *tempString = NULL;

		if ((i + 1) >= nextTicCount)
		{
			size_t ticsNeeded = (size_t)(((double)(i + 1) / numPermutations) * 50.0);

			do
			{
				ri.Printf(PRINT_ALL, "*");
			}
			while (++tics < ticsNeeded);

			nextTicCount = (size_t)((tics / 50.0) * numPermutations);

			if (i == (numPermutations - 1))
			{
				if (tics < 51)
				{
					ri.Printf(PRINT_ALL, "*");
				}

				ri.Printf(PRINT_ALL, "\n");
			}
		}

		if (GLSL_GenerateMacroString(info->list, info->extraMacros, i, &tempString))
		{
			if (GLSL_GetProgramPermutation(info, i, vertexShader, fragmentShader, tempString))
			{
				//Set uniform values
				GLSL_SetInitialUniformValues(info, i);
				GLSL_FinishGPUShader(&info->list->programs[i]);
				info->list->programs[i].compiled = qtrue;
			}
			else
			{
				ri.Error(ERR_FATAL, "Failed to compile shader: %s permutation %d\n", info->name, i);
			}

			numCompiled++;
		}
		else
		{
			info->list->programs[i].program  = 0;
			info->list->programs[i].compiled = qfalse;
		}

	}

	endTime = ri.Milliseconds();
	ri.Printf(PRINT_ALL, "...compiled %i %s shader permutations in %5.2f seconds\n", ( int ) numCompiled, info->name, (endTime - startTime) / 1000.0);
	info->compiled                 = qtrue;
	info->list->currentPermutation = 0;
	return qtrue;
}

programInfo_t *GLSL_GetShaderProgram(const char *name)
{
	programInfo_t *prog;

	prog = GLSL_FindShader(name);

	if (prog && !prog->compiled)
	{
		//Compile the shader program
		GLSL_CompileShaderProgram(prog);
	}

	return prog;
}

void GLSL_SetMacroStatesByOffset(programInfo_t *programlist, int offset)
{
	if (offset < 0)
	{
		ri.Error(ERR_FATAL, "Trying to select an negative array cell\n");
		return;
	}

	programlist->list->currentPermutation = 0;
	programlist->list->currentMacros      = 0;

	if (offset != 0)
	{
		int i = 0;
		programlist->list->currentPermutation = offset;
		for (; i < MAX_MACROS; i++)
		{
			if (BIT(programlist->list->macromap[i]) != -1)
			{
				if (offset & BIT(programlist->list->macromap[i]))
				{
					programlist->list->currentMacros |= BIT(i);
				}
			}
		}
	}
}

void GLSL_SetMacroState(programInfo_t *programlist, int macro, int enabled)
{
	if (!programlist)
	{
		ri.Error(ERR_FATAL, "GLSL_SetMacroState: NULL programinfo");
	}

	if (!programlist->compiled)
	{
		ri.Error(ERR_FATAL, "Trying to set macro state of shader \"%s\" but it is not compiled\n", programlist->name);
	}

	if (programlist->list->macromap[macro] == -1)
	{
		return;
	}

	if (enabled > 0 && (programlist->list->currentMacros & BIT(macro)))
	{
		return;
	}
	else if (enabled <= 0 && !(programlist->list->currentMacros & BIT(macro)))
	{
		return;
	}

	if (enabled > 0)
	{
		programlist->list->currentPermutation |= BIT(programlist->list->macromap[macro]);
		programlist->list->currentMacros      |= BIT(macro);
	}
	else
	{
		programlist->list->currentPermutation &= ~BIT(programlist->list->macromap[macro]);
		programlist->list->currentMacros      &= ~BIT(macro);
	}

	if (programlist->list->permutations < programlist->list->currentPermutation)
	{
		ri.Error(ERR_FATAL, "GLSL_SetMacroState: Trying to set macro state to impossible result for shader: %s with macro: %i permutation number %i", programlist->name, macro, programlist->list->permutations);
	}
}

void GLSL_SetMacroStates(programInfo_t *programlist, int numMacros, ...)
{
	int     macro, value;
	va_list ap;

	//Reset the macro states
	GLSL_SetMacroStatesByOffset(programlist, 0);

	if (numMacros == 0)
	{
		return;
	}

	if (numMacros == 1)
	{
		va_start(ap, numMacros);
		value = va_arg(ap, int);
		GLSL_SetMacroStatesByOffset(programlist, value);
		return;
	}

	if (numMacros % 2 != 0)
	{
		ri.Error(ERR_FATAL, "GLSL_SetMacroStates: Trying to set macros with an array which has an invalid size %i\n", numMacros);
		return;
	}

	va_start(ap, numMacros);
	do
	{
		macro = va_arg(ap, int);
		value = va_arg(ap, int);

		GLSL_SetMacroState(programlist, macro, value);

		numMacros -= 2;
	}
	while (numMacros > 0);
	va_end(ap);
}

void GLSL_SelectPermutation(programInfo_t *programlist)
{
	shaderProgram_t *prog;

	if (!programlist)
	{
		ri.Error(ERR_FATAL, "GLSL_SelectPermutation: NULL programinfo");
	}

	if (!programlist->compiled)
	{
		ri.Error(ERR_FATAL, "Trying to select permutation of shader \"%s\" but the list is not compiled\n", programlist->name);
	}

	prog = &programlist->list->programs[programlist->list->currentPermutation];

	if (!prog || !prog->compiled)
	{
		ri.Error(ERR_FATAL, "Trying to select uncompiled shader permutation: %d of shader \"%s\"\n", programlist->list->currentPermutation, programlist->name);
	}
	else
	{
		selectedProgram = programlist->list->current = prog;
		GLSL_BindProgram(prog);
	}
}

void GLSL_SetRequiredVertexPointers(programInfo_t *programlist)
{
	//FIXME: implement this
	//see void GLShader::SetRequiredVertexPointers() in gl_shader.cpp
	/*
	uint32_t macroVertexAttribs = 0;
	size_t   numMacros          = _compileMacros.size();

	for (size_t j = 0; j < numMacros; j++)
	{
	    GLCompileMacro *macro = _compileMacros[j];

	    int bit = macro->GetBit();

	    if (IsMacroSet(bit))
	    {
	        macroVertexAttribs |= macro->GetRequiredVertexAttributes();
	    }
	}

	GLSL_VertexAttribsState((_vertexAttribsRequired | _vertexAttribs | macroVertexAttribs));      // & ~_vertexAttribsUnsupported);
	*/
	uint32_t macroVertexAttribs = 0;
	int      j;

	for (j = 0; j < programlist->numMacros; j++)
	{
		int macro = programlist->macros[j];

		if (programlist->list->currentMacros & BIT(macro))
		{
			macroVertexAttribs |= GLSL_GetRequiredVertexAttributes(macro);
		}
	}

	GLSL_VertexAttribsState((programlist->attributes | macroVertexAttribs));
}

void GLSL_DeleteGPUShader(shaderProgram_t *program)
{
	if (program->program)
	{
		if (program->vertexShader)
		{
			qglDetachObjectARB(program->program, program->vertexShader);
			qglDeleteObjectARB(program->vertexShader);
		}

		if (program->fragmentShader)
		{
			qglDetachObjectARB(program->program, program->fragmentShader);
			qglDeleteObjectARB(program->fragmentShader);
		}

		qglDeleteObjectARB(program->program);

		if (program->uniformBuffer)
		{
			Com_Dealloc(program->uniformBuffer);
		}

		Com_Memset(program, 0, sizeof(*program));
	}
}

void GLSL_DeleteShaderProgramList(shaderProgramList_t *programlist)
{
	int i;

	for (i = 0; i < programlist->permutations; i++)
	{
		GLSL_DeleteGPUShader(&programlist->programs[i]);
	}

	Com_Dealloc(programlist->programs);
}

void GLSL_DeleteShaderProramInfo(programInfo_t *program)
{
	if (program->list)
	{
		GLSL_DeleteShaderProgramList(program->list);
	}

	if (program->extraMacros)
	{
		Com_Dealloc(program->extraMacros);
	}

	if (program->filename)
	{
		Com_Dealloc(program->filename);
	}

	if (program->fragFilename)
	{
		Com_Dealloc(program->fragFilename);
	}

	if (program->fragmentLibraries)
	{
		Com_Dealloc(program->fragmentLibraries);
	}

	if (program->name)
	{
		Com_Dealloc(program->name);
	}

	if (program->vertexLibraries)
	{
		Com_Dealloc(program->vertexLibraries);
	}
}

void GLSL_InitGPUShaders(void)
{
	int startTime, endTime;

	ri.Printf(PRINT_ALL, "------- GLSL_InitGPUShaders -------\n");

	R_IssuePendingRenderCommands();

	startTime = ri.Milliseconds();

	//Load all definitions
	GLSL_LoadDefinitions();

	gl_genericShader      = GLSL_GetShaderProgram("generic");
	gl_lightMappingShader = GLSL_GetShaderProgram("lightMapping");

	gl_vertexLightingShader_DBS_entity = GLSL_GetShaderProgram("vertexLighting_DBS_entity");

	gl_vertexLightingShader_DBS_world = GLSL_GetShaderProgram("vertexLighting_DBS_world");

	if (DS_STANDARD_ENABLED())
	{
		gl_geometricFillShader                   = GLSL_GetShaderProgram("geometricFill");
		gl_deferredLightingShader_omniXYZ        = GLSL_GetShaderProgram("deferredLighting_omniXYZ");
		gl_deferredLightingShader_projXYZ        = GLSL_GetShaderProgram("deferredLighting_projXYZ");
		gl_deferredLightingShader_directionalSun = GLSL_GetShaderProgram("deferredLighting_directionalSun");
	}
	else
	{
		gl_forwardLightingShader_omniXYZ        = GLSL_GetShaderProgram("forwardLighting_omniXYZ");
		gl_forwardLightingShader_projXYZ        = GLSL_GetShaderProgram("forwardLighting_projXYZ");
		gl_forwardLightingShader_directionalSun = GLSL_GetShaderProgram("forwardLighting_directionalSun");
	}

	gl_shadowFillShader     = GLSL_GetShaderProgram("shadowFill");
	gl_reflectionShader     = GLSL_GetShaderProgram("reflection");
	gl_skyboxShader         = GLSL_GetShaderProgram("skybox");
	gl_fogQuake3Shader      = GLSL_GetShaderProgram("fogQuake3");
	gl_fogGlobalShader      = GLSL_GetShaderProgram("fogGlobal");
	gl_heatHazeShader       = GLSL_GetShaderProgram("heatHaze");
	gl_screenShader         = GLSL_GetShaderProgram("screen");
	gl_portalShader         = GLSL_GetShaderProgram("portal");
	gl_toneMappingShader    = GLSL_GetShaderProgram("toneMapping");
	gl_contrastShader       = GLSL_GetShaderProgram("contrast");
	gl_cameraEffectsShader  = GLSL_GetShaderProgram("cameraEffects");
	gl_blurXShader          = GLSL_GetShaderProgram("blurX");
	gl_blurYShader          = GLSL_GetShaderProgram("blurY");
	gl_debugShadowMapShader = GLSL_GetShaderProgram("debugShadowMap");

	//Dushan
	gl_liquidShader             = GLSL_GetShaderProgram("liquid");
	gl_rotoscopeShader          = GLSL_GetShaderProgram("rotoscope");
	gl_bloomShader              = GLSL_GetShaderProgram("bloom");
	gl_refractionShader         = GLSL_GetShaderProgram("refraction");
	gl_depthToColorShader       = GLSL_GetShaderProgram("depthToColor");
	gl_volumetricFogShader      = GLSL_GetShaderProgram("volumetricFog");
	gl_volumetricLightingShader = GLSL_GetShaderProgram("lightVolume_omni");
	gl_dispersionShader         = GLSL_GetShaderProgram("dispersion");

	gl_depthOfField = GLSL_GetShaderProgram("depthOfField");
	gl_ssao         = GLSL_GetShaderProgram("SSAO");

	//Jacker
	gl_colorCorrection = GLSL_GetShaderProgram("colorCorrection");

	endTime = ri.Milliseconds();

	ri.Printf(PRINT_ALL, "Created default shaders in %5.2f seconds\n", (endTime - startTime) / 1000.0);

	if (r_recompileShaders->integer)
	{
		ri.Cvar_Set("r_recompileShaders", "0");
	}
}

void GLSL_ShutdownGPUShaders(void)
{
	int i;

	ri.Printf(PRINT_ALL, "------- GLSL_ShutdownGPUShaders -------\n");

	//GLSL_VertexAttribsState(0);
	GLSL_BindNullProgram();

	//Clean up programInfo_t:s
	for (i = 0; i < FILE_HASH_SIZE; i++)
	{
		if (hashTable[i])
		{
			programInfo_t *prog = hashTable[i];
			GLSL_DeleteShaderProramInfo(prog);
			Com_Dealloc(prog);
			hashTable[i] = NULL;
		}
	}

	Com_Dealloc(definitionText);

	glState.currentProgram = 0;
	qglUseProgramObjectARB(0);
}


void GLSL_BindProgram(shaderProgram_t *program)
{
	if (!program)
	{
		GLSL_BindNullProgram();
		return;
	}

	Ren_LogComment("--- GL_BindProgram( %s ) ---\n", program->name);

	if (glState.currentProgram != program)
	{
		qglUseProgramObjectARB(program->program);
		glState.currentProgram = program;
		backEnd.pc.c_glslShaderBinds++;
	}
}


void GLSL_BindNullProgram(void)
{
	Ren_LogComment("--- GL_BindNullProgram ---\n");

	if (glState.currentProgram)
	{
		qglUseProgramObjectARB(0);
		glState.currentProgram = NULL;
	}
}

void GLSL_SetUniform_DeformParms(deformStage_t deforms[], int numDeforms)
{
	float deformParms[MAX_SHADER_DEFORM_PARMS];
	int   deformOfs = 0, i;

	if (numDeforms > MAX_SHADER_DEFORMS)
	{
		numDeforms = MAX_SHADER_DEFORMS;
	}

	deformParms[deformOfs++] = numDeforms;

	for (i = 0; i < numDeforms; i++)
	{
		deformStage_t *ds = &deforms[i];

		switch (ds->deformation)
		{
		case DEFORM_WAVE:
			deformParms[deformOfs++] = DEFORM_WAVE;

			deformParms[deformOfs++] = ds->deformationWave.func;
			deformParms[deformOfs++] = ds->deformationWave.base;
			deformParms[deformOfs++] = ds->deformationWave.amplitude;
			deformParms[deformOfs++] = ds->deformationWave.phase;
			deformParms[deformOfs++] = ds->deformationWave.frequency;

			deformParms[deformOfs++] = ds->deformationSpread;
			break;

		case DEFORM_BULGE:
			deformParms[deformOfs++] = DEFORM_BULGE;

			deformParms[deformOfs++] = ds->bulgeWidth;
			deformParms[deformOfs++] = ds->bulgeHeight;
			deformParms[deformOfs++] = ds->bulgeSpeed;
			break;

		case DEFORM_MOVE:
			deformParms[deformOfs++] = DEFORM_MOVE;

			deformParms[deformOfs++] = ds->deformationWave.func;
			deformParms[deformOfs++] = ds->deformationWave.base;
			deformParms[deformOfs++] = ds->deformationWave.amplitude;
			deformParms[deformOfs++] = ds->deformationWave.phase;
			deformParms[deformOfs++] = ds->deformationWave.frequency;

			deformParms[deformOfs++] = ds->bulgeWidth;
			deformParms[deformOfs++] = ds->bulgeHeight;
			deformParms[deformOfs++] = ds->bulgeSpeed;
			break;

		default:
			break;
		}
		GLSL_SetUniformFloatARR(selectedProgram, UNIFORM_DEFORMPARMS, deformParms, MAX_SHADER_DEFORM_PARMS);
	}
}

void GLSL_SetUniform_ColorModulate(programInfo_t *prog, int colorGen, int alphaGen)
{
	vec4_t temp;
	switch (colorGen)
	{
	case CGEN_VERTEX:
		prog->attributes |= ATTR_COLOR;
		VectorSet(temp, 1, 1, 1);
		break;

	case CGEN_ONE_MINUS_VERTEX:
		prog->attributes |= ATTR_COLOR;
		VectorSet(temp, -1, -1, -1);
		break;

	default:
		prog->attributes &= ~ATTR_COLOR;
		VectorSet(temp, 0, 0, 0);
		break;
	}

	switch (alphaGen)
	{
	case AGEN_VERTEX:
		prog->attributes |= ATTR_COLOR;
		temp[3]           = 1.0f;
		break;

	case AGEN_ONE_MINUS_VERTEX:
		prog->attributes |= ATTR_COLOR;
		temp[3]           = -1.0f;
		break;

	default:
		temp[3] = 0.0f;
		break;
	}

	if (prog->attributes & ATTR_COLOR && !(glState.vertexAttribsState & ATTR_COLOR))
	{
		glEnableVertexAttribArray(ATTR_INDEX_COLOR);
		glState.vertexAttribsState |= ATTR_COLOR;
		glVertexAttribPointer(ATTR_INDEX_COLOR, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsColors));
		glState.vertexAttribPointersSet |= ATTR_COLOR;
	}
	else if (!(prog->attributes & ATTR_COLOR) && glState.vertexAttribsState & ATTR_COLOR)
	{
		glDisableVertexAttribArray(ATTR_INDEX_COLOR);
		glState.vertexAttribsState &= ~ATTR_COLOR;
	}

	SetUniformVec4(UNIFORM_COLORMODULATE, temp);
}

void GLSL_SetUniform_AlphaTest(uint32_t stateBits)
{
	alphaTest_t value;

	switch (stateBits & GLS_ATEST_BITS)
	{
	case GLS_ATEST_GT_0:
		value = ATEST_GT_0;
		break;

	case GLS_ATEST_LT_128:
		value = ATEST_LT_128;
		break;

	case GLS_ATEST_GE_128:
		value = ATEST_GE_128;
		break;

	default:
		value = ATEST_NONE;
		break;
	}

	SetUniformInt(UNIFORM_ALPHATEST, value);
}

void GLSL_VertexAttribsState(uint32_t stateBits)
{
	uint32_t diff;

	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		stateBits |= (ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS);
	}

	GLSL_VertexAttribPointers(stateBits);

	diff = stateBits ^ glState.vertexAttribsState;
	if (!diff)
	{
		return;
	}

	if (diff & ATTR_POSITION)
	{
		if (stateBits & ATTR_POSITION)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_POSITION )\n");
			glEnableVertexAttribArray(ATTR_INDEX_POSITION);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_POSITION )\n");
			glDisableVertexAttribArray(ATTR_INDEX_POSITION);
		}
	}

	if (diff & ATTR_TEXCOORD)
	{
		if (stateBits & ATTR_TEXCOORD)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_TEXCOORD )\n");
			glEnableVertexAttribArray(ATTR_INDEX_TEXCOORD0);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_TEXCOORD )\n");
			glDisableVertexAttribArray(ATTR_INDEX_TEXCOORD0);
		}
	}

	if (diff & ATTR_LIGHTCOORD)
	{
		if (stateBits & ATTR_LIGHTCOORD)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_LIGHTCOORD )\n");
			glEnableVertexAttribArray(ATTR_INDEX_TEXCOORD1);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_LIGHTCOORD )\n");
			glDisableVertexAttribArray(ATTR_INDEX_TEXCOORD1);
		}
	}

	if (diff & ATTR_TANGENT)
	{
		if (stateBits & ATTR_TANGENT)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_TANGENT )\n");
			glEnableVertexAttribArray(ATTR_INDEX_TANGENT);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_TANGENT )\n");
			glDisableVertexAttribArray(ATTR_INDEX_TANGENT);
		}
	}

	if (diff & ATTR_BINORMAL)
	{
		if (stateBits & ATTR_BINORMAL)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_BINORMAL )\n");
			glEnableVertexAttribArray(ATTR_INDEX_BINORMAL);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_BINORMAL )\n");
			glDisableVertexAttribArray(ATTR_INDEX_BINORMAL);
		}
	}

	if (diff & ATTR_NORMAL)
	{
		if (stateBits & ATTR_NORMAL)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_NORMAL )\n");
			glEnableVertexAttribArray(ATTR_INDEX_NORMAL);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_NORMAL )\n");
			glDisableVertexAttribArray(ATTR_INDEX_NORMAL);
		}
	}

	if (diff & ATTR_COLOR)
	{
		if (stateBits & ATTR_COLOR)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_COLOR )\n");
			glEnableVertexAttribArray(ATTR_INDEX_COLOR);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_COLOR )\n");
			glDisableVertexAttribArray(ATTR_INDEX_COLOR);
		}
	}

	if (diff & ATTR_BONE_INDEXES)
	{
		if (stateBits & ATTR_BONE_INDEXES)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_BONE_INDEXES )\n");
			glEnableVertexAttribArray(ATTR_INDEX_BONE_INDEXES);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_BONE_INDEXES )\n");
			glDisableVertexAttribArray(ATTR_INDEX_BONE_INDEXES);
		}
	}

	if (diff & ATTR_BONE_WEIGHTS)
	{
		if (stateBits & ATTR_BONE_WEIGHTS)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_BONE_WEIGHTS )\n");
			glEnableVertexAttribArray(ATTR_INDEX_BONE_WEIGHTS);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_BONE_WEIGHTS )\n");
			glDisableVertexAttribArray(ATTR_INDEX_BONE_WEIGHTS);
		}
	}

	if (diff & ATTR_POSITION2)
	{
		if (stateBits & ATTR_POSITION2)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_POSITION2 )\n");
			glEnableVertexAttribArray(ATTR_INDEX_POSITION2);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_POSITION2 )\n");
			glDisableVertexAttribArray(ATTR_INDEX_POSITION2);
		}
	}

	if (diff & ATTR_TANGENT2)
	{
		if (stateBits & ATTR_TANGENT2)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_TANGENT2 )\n");
			glEnableVertexAttribArray(ATTR_INDEX_TANGENT2);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_TANGENT2 )\n");
			glDisableVertexAttribArray(ATTR_INDEX_TANGENT2);
		}
	}

	if (diff & ATTR_BINORMAL2)
	{
		if (stateBits & ATTR_BINORMAL2)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_BINORMAL2 )\n");
			glEnableVertexAttribArray(ATTR_INDEX_BINORMAL2);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_BINORMAL2 )\n");
			glDisableVertexAttribArray(ATTR_INDEX_BINORMAL2);
		}
	}

	if (diff & ATTR_NORMAL2)
	{
		if (stateBits & ATTR_NORMAL2)
		{
			Ren_LogComment("glEnableVertexAttribArray( ATTR_INDEX_NORMAL2 )\n");
			glEnableVertexAttribArray(ATTR_INDEX_NORMAL2);
		}
		else
		{
			Ren_LogComment("glDisableVertexAttribArray( ATTR_INDEX_NORMAL2 )\n");
			glDisableVertexAttribArray(ATTR_INDEX_NORMAL2);
		}
	}

	glState.vertexAttribsState = stateBits;
}

void GLSL_VertexAttribPointers(uint32_t attribBits)
{
	if (!glState.currentVBO)
	{
		ri.Error(ERR_FATAL, "GL_VertexAttribPointers: no VBO bound");
		return;
	}

	Ren_LogComment("--- GL_VertexAttribPointers( %s ) ---\n", glState.currentVBO->name);

	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		attribBits |= (ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS);
	}

	if ((attribBits & ATTR_POSITION))
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_POSITION )\n");

		glVertexAttribPointer(ATTR_INDEX_POSITION, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsXYZ + (glState.vertexAttribsOldFrame * glState.currentVBO->sizeXYZ)));
		glState.vertexAttribPointersSet |= ATTR_POSITION;
	}

	if ((attribBits & ATTR_TEXCOORD))
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_TEXCOORD )\n");

		glVertexAttribPointer(ATTR_INDEX_TEXCOORD0, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsTexCoords));
		glState.vertexAttribPointersSet |= ATTR_TEXCOORD;
	}

	if ((attribBits & ATTR_LIGHTCOORD))
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_LIGHTCOORD )\n");

		glVertexAttribPointer(ATTR_INDEX_TEXCOORD1, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsLightCoords));
		glState.vertexAttribPointersSet |= ATTR_LIGHTCOORD;
	}

	if ((attribBits & ATTR_TANGENT))
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_TANGENT )\n");

		glVertexAttribPointer(ATTR_INDEX_TANGENT, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsTangents + (glState.vertexAttribsOldFrame * glState.currentVBO->sizeTangents)));
		glState.vertexAttribPointersSet |= ATTR_TANGENT;
	}

	if ((attribBits & ATTR_BINORMAL))
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_BINORMAL )\n");

		glVertexAttribPointer(ATTR_INDEX_BINORMAL, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsBinormals + (glState.vertexAttribsOldFrame * glState.currentVBO->sizeBinormals)));
		glState.vertexAttribPointersSet |= ATTR_BINORMAL;
	}

	if ((attribBits & ATTR_NORMAL))
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_NORMAL )\n");

		glVertexAttribPointer(ATTR_INDEX_NORMAL, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsNormals + (glState.vertexAttribsOldFrame * glState.currentVBO->sizeNormals)));
		glState.vertexAttribPointersSet |= ATTR_NORMAL;
	}

	if ((attribBits & ATTR_COLOR))
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_COLOR )\n");

		glVertexAttribPointer(ATTR_INDEX_COLOR, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsColors));
		glState.vertexAttribPointersSet |= ATTR_COLOR;
	}

	if ((attribBits & ATTR_BONE_INDEXES))
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_BONE_INDEXES )\n");

		glVertexAttribPointer(ATTR_INDEX_BONE_INDEXES, 4, GL_INT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsBoneIndexes));
		glState.vertexAttribPointersSet |= ATTR_BONE_INDEXES;
	}

	if ((attribBits & ATTR_BONE_WEIGHTS))
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_BONE_WEIGHTS )\n");

		glVertexAttribPointer(ATTR_INDEX_BONE_WEIGHTS, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsBoneWeights));
		glState.vertexAttribPointersSet |= ATTR_BONE_WEIGHTS;
	}

	if (glState.vertexAttribsInterpolation > 0)
	{
		if ((attribBits & ATTR_POSITION2))
		{
			Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_POSITION2 )\n");

			glVertexAttribPointer(ATTR_INDEX_POSITION2, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsXYZ + (glState.vertexAttribsNewFrame * glState.currentVBO->sizeXYZ)));
			glState.vertexAttribPointersSet |= ATTR_POSITION2;
		}

		if ((attribBits & ATTR_TANGENT2))
		{
			Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_TANGENT2 )\n");

			glVertexAttribPointer(ATTR_INDEX_TANGENT2, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsTangents + (glState.vertexAttribsNewFrame * glState.currentVBO->sizeTangents)));
			glState.vertexAttribPointersSet |= ATTR_TANGENT2;
		}

		if ((attribBits & ATTR_BINORMAL2))
		{
			Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_BINORMAL2 )\n");

			glVertexAttribPointer(ATTR_INDEX_BINORMAL2, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsBinormals + (glState.vertexAttribsNewFrame * glState.currentVBO->sizeBinormals)));
			glState.vertexAttribPointersSet |= ATTR_BINORMAL2;
		}

		if ((attribBits & ATTR_NORMAL2))
		{
			Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_NORMAL2 )\n");

			glVertexAttribPointer(ATTR_INDEX_NORMAL2, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsNormals + (glState.vertexAttribsNewFrame * glState.currentVBO->sizeNormals)));
			glState.vertexAttribPointersSet |= ATTR_NORMAL2;
		}
	}
}
