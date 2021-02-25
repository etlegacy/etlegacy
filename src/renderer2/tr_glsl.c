/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * XreaL
 * Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
 *
 * This file is part of ET: Legacy - http://www.etlegacy.com
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file renderer2/tr_glsl.c
 */

#include "tr_local.h"

#include <shaders.h>

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

const int textureMap[] =
{
#define TEX_MAP
#include "tr_glsldef.h"
#undef TEX_MAP
};

const int numberofAttributes = ARRAY_LEN(attributeMap);

#define FILE_HASH_SIZE      4096
#define MAX_SHADER_DEF_FILES 1024
#define DEFAULT_SHADER_DEF NULL
static programInfo_t *hashTable[FILE_HASH_SIZE];

static char *definitionText;

#define GL_SHADER_VERSION 3

/**
 * @struct GLShaderHeader_s
 * @typedef GLShaderHeader_t
 * @brief
 */
typedef struct GLShaderHeader_s
{
	unsigned int version;
	unsigned int checkSum;              ///< checksum of shader source this was built from

	unsigned int macros[MAX_MACROS];    ///< macros the shader uses ( may or may not be enabled )
	unsigned int numMacros;

	GLenum binaryFormat;                ///< argument to glProgramBinary
	GLint binaryLength;                 ///< argument to glProgramBinary
}GLShaderHeader_t;

/*
enum GLShaderTypeEnum
{
    LEGACY_VERTEX,
    LEGACY_FRAGMENT,
    LEGACY_GEOMETRY,
    LEGACY_TESS_CONTROL,
    LEGACY_TESS_EVAL,
};

typedef struct GLShaderType_s
{
    unsigned int type;
    unsigned int gltype;
    char *typetext;
    char *extension;
} GLShaderType_t;

const GLShaderType_t shaderTypes[] =
{
    { LEGACY_VERTEX,       GL_VERTEX_SHADER,          "vert",     "_vp.glsl"  },
    { LEGACY_FRAGMENT,     GL_FRAGMENT_SHADER,        "frag",     "_fp.glsl"  },
    { LEGACY_GEOMETRY,     GL_GEOMETRY_SHADER,        "geom",     "_gm.glsl"  },
    { LEGACY_TESS_CONTROL, GL_TESS_CONTROL_SHADER,    "tesscont", "_tsc.glsl" },
    { LEGACY_TESS_EVAL,    GL_TESS_EVALUATION_SHADER, "tesseval", "_tse.glsl" },
};

const int numberofshaderTypes = ARRAY_LEN(shaderTypes);
*/

trPrograms_t trProg;

/**
 * @brief GLSL_GetMacroByName
 * @param[in] name
 * @return
 */
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

/**
 * @brief GLSL_GetAttribByName
 * @param[in] name
 * @return
 *
 * @todo FIXME: return -1 but type is unsigned !
 */
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

	Ren_Warning("GLSL_GetAttribByName Warning: No attribute '%s' found.\n", name);

	return -1;
}

/**
 * @brief GLSL_CopyStringAlloc
 * @param[out] out
 * @param[in] in
 */
void GLSL_CopyStringAlloc(char **out, const char *in)
{
	size_t size = strlen(in) * sizeof(char) + 1;

	*out = (char *) Com_Allocate(size);
	Com_Memset(*out, '\0', size);
	Q_strncpyz(*out, in, size);
}

/**
 * @brief GLSL_CopyNextToken
 * @param[in,out] text
 * @param[out] out
 * @return
 */
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

/**
 * @brief GLSL_ParseDefinition
 * @param[in,out] text
 * @param[in] defname
 * @return
 */
programInfo_t *GLSL_ParseDefinition(char **text, const char *defname)
{
	char          *token;
	programInfo_t *def;
	void          *valptr;

	token = COM_ParseExt(text, qtrue);
	if (token[0] != '{')
	{
		Ren_Warning("WARNING: expecting '{', found '%s' instead in shader definition '%s'\n", token, defname);
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
			Ren_Warning("WARNING: no concluding '}' in shader definition %s\n", defname);
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
					Ren_Warning("WARNING: Macro '%s' for shaderdef '%s' was not recognized\n", token, defname);
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
				if (def->numUniformValues < MAX_UNIFORM_VALUES - 1)
				{
					def->uniformValues[def->numUniformValues].type.type = GLSL_INT;
					GLSL_CopyNextToken(text, &def->uniformValues[def->numUniformValues].type.name);
					token                                           = COM_ParseExt(text, qtrue);
					valptr                                          = Com_Allocate(sizeof(int));
					*((int *)valptr)                                = Q_atoi(token);
					def->uniformValues[def->numUniformValues].value = valptr;
					//Ren_Print("%d\n",*((int*)valptr));
					def->numUniformValues++;
				}
				else
				{
					Ren_Warning("GLSL_ParseDefinition: MAX_UNIFORM_VALUES reached.\n");
					goto parseerror;
				}
			}
			else if (!Q_stricmp(token, "bool"))
			{
				if (def->numUniformValues < MAX_UNIFORM_VALUES - 1)
				{
					def->uniformValues[def->numUniformValues].type.type = GLSL_BOOL;
					GLSL_CopyNextToken(text, &def->uniformValues[def->numUniformValues].type.name);
					token                                           = COM_ParseExt(text, qtrue);
					valptr                                          = Com_Allocate(sizeof(qboolean));
					*((qboolean *)valptr)                           = Q_atoi(token);
					def->uniformValues[def->numUniformValues].value = valptr;
					//Ren_Print("%d\n",*((qboolean*)valptr));
					def->numUniformValues++;
				}
				else
				{
					Ren_Warning("GLSL_ParseDefinition: MAX_UNIFORM_VALUES reached.\n");
					goto parseerror;
				}
			}
			else
			{
				// FIXME: implement other formats
				Ren_Warning("GLSL_ParseDefinition: uniform format not implemented.\n");
				goto parseerror;
			}
		}
	}

	return def;

parseerror:
	Com_Dealloc(def);
	return NULL;
}

/**
 * @brief GLSL_FindDefinitionInText
 * @param[in] shadername
 * @return
 */
static char *GLSL_FindDefinitionInText(const char *shadername)
{
	char *p = definitionText;
	char *token;

	if (!p)
	{
		Ren_Fatal("GLSL_FindDefinitionInText: Definition text is null");
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

/**
 * @brief GLSL_FindShader
 * @param[in] name
 * @return
 */
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

	hash = Q_GenerateHashValue(strippedName, FILE_HASH_SIZE, qfalse, qtrue);

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
		Ren_Print("Shader definition find failed: %s\n", name);
		return DEFAULT_SHADER_DEF;
	}

	sh = GLSL_ParseDefinition(&shaderText, strippedName);
	if (!sh)
	{
		Ren_Print("Shader definition parsing failed: %s\n", name);
		return DEFAULT_SHADER_DEF;
	}
	else
	{
		sh->next        = hashTable[hash];
		hashTable[hash] = sh;
		return sh;
	}
}

/**
 * @brief GLSL_LoadDefinitions
 */
void GLSL_LoadDefinitions(void)
{
	// FIXME: Also load from external files in the future...
	// For now just copy the existing data to our search able string
	const char *defaultShaderDef = GetFallbackShaderDef();
	size_t     size              = strlen(defaultShaderDef) * sizeof(char);

	definitionText = (char *)Com_Allocate(size + 1);
	Com_Memset(definitionText, '\0', size + 1);
	Q_strncpyz(definitionText, defaultShaderDef, size);
}

/**
 * @brief GLSL_PrintInfoLog
 * @param[in] object
 * @param[in] developerOnly
 * @param[in] isProgram
 */
static void GLSL_PrintInfoLog(GLhandleARB object, qboolean developerOnly, qboolean isProgram)
{
	char        *msg;
	static char msgPart[1024];
	int         maxLength = 0;
	int         i;
	int         printLevel = developerOnly ? PRINT_DEVELOPER : PRINT_ALL;

	if (isProgram)
	{
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &maxLength);
	}
	else
	{
		glGetShaderiv(object, GL_INFO_LOG_LENGTH, &maxLength);
	}

	if (maxLength <= 0)
	{
		ri.Printf(printLevel, "No compile log.\n");
		return;
	}

	ri.Printf(printLevel, "compile log:\n");

	if (maxLength < 1023)
	{
		if (isProgram)
		{
			glGetProgramInfoLog(object, maxLength, &maxLength, msgPart);
		}
		else
		{
			glGetShaderInfoLog(object, maxLength, &maxLength, msgPart);
		}

		msgPart[maxLength + 1] = '\0';

		ri.Printf(printLevel, "%s\n", msgPart);
	}
	else
	{
		msg = ri.Hunk_AllocateTempMemory(maxLength);

		if (isProgram)
		{
			glGetProgramInfoLog(object, maxLength, &maxLength, msg);
		}
		else
		{
			glGetShaderInfoLog(object, maxLength, &maxLength, msg);
		}

		for (i = 0; i < maxLength; i += 1024)
		{
			Q_strncpyz(msgPart, msg + i, sizeof(msgPart));

			ri.Printf(printLevel, "%s\n", msgPart);
		}
		ri.Hunk_FreeTempMemory(msg);
	}
}

#if 0
/**
 * @brief GLSL_PrintShaderSource
 * @param[in] object
 */
static void GLSL_PrintShaderSource(GLhandleARB object)
{
	char        *msg;
	static char msgPart[1024];
	int         maxLength = 0;
	int         i;

	glGetShaderiv(object, GL_SHADER_SOURCE_LENGTH, &maxLength);

	msg = ri.Hunk_AllocateTempMemory(maxLength);

	glGetShaderSource(object, maxLength, &maxLength, msg);

	for (i = 0; i < maxLength; i += 1024)
	{
		Q_strncpyz(msgPart, msg + i, sizeof(msgPart));
		Ren_Print("%s\n", msgPart);
	}

	ri.Hunk_FreeTempMemory(msg);
}
#endif

/**
 * @brief GLSL_LoadShaderBinary
 * @param[in] info
 * @param[in] programNum
 * @return
 */
qboolean GLSL_LoadShaderBinary(programInfo_t *info, size_t programNum)
{
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
	Com_Memcpy(&shaderHeader, binaryptr, sizeof(shaderHeader));
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
	shaderProgram->program = glCreateProgram();
	glProgramBinary(shaderProgram->program, shaderHeader.binaryFormat, binaryptr, shaderHeader.binaryLength);
	glGetProgramiv(shaderProgram->program, GL_LINK_STATUS, &success);

	if (!success)
	{
		ri.FS_FreeFile(binary);
		glDeleteProgram(shaderProgram->program);
		return qfalse;
	}

	ri.FS_FreeFile(binary);
	return qtrue;
}

/**
 * @brief GLSL_SaveShaderBinary
 * @param[in] info
 * @param[in] programNum
 */
void GLSL_SaveShaderBinary(programInfo_t *info, size_t programNum)
{
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

	Com_Memset(&shaderHeader, 0, sizeof(shaderHeader));

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
	Com_Memcpy(( void * )binary, &shaderHeader, sizeof(shaderHeader));

	ri.FS_WriteFile(va("glsl/%s/%s_%u.bin", info->name, info->name, ( unsigned int ) programNum), binary, binarySize);

	ri.Hunk_FreeTempMemory(binary);
}

/**
 * @brief GLSL_HasConflictingMacros
 * @param[in] compilemacro
 * @param[in] usedmacros
 * @return
 */
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

/**
 * @brief GLSL_MissesRequiredMacros
 * @param[in] compilemacro
 * @param[in] usedmacros
 * @return
 */
static qboolean GLSL_MissesRequiredMacros(int compilemacro, int usedmacros)
{
	switch (compilemacro)
	{
	case USE_PARALLAX_MAPPING:
	case USE_DELUXE_MAPPING:
	case USE_REFLECTIONS:
	case USE_SPECULAR:
		if (!(usedmacros & BIT(USE_NORMAL_MAPPING)))
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

/**
 * @brief GLSL_GetRequiredVertexAttributes
 * @param[in] compilemacro
 * @return
 */
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

static char shaderExtraDef[8000];
//static int  shaderExtraDefLen = 0;

/**
 * @brief GLSL_BuildShaderExtraDef
 */
static void GLSL_BuildShaderExtraDef()
{
	float fbufWidthScale, fbufHeightScale;
	float npotWidthScale, npotHeightScale;

	Com_Memset(shaderExtraDef, 0, sizeof(shaderExtraDef));

#define BUFFEXT(...) Q_strcat(shaderExtraDef, sizeof(shaderExtraDef), va(__VA_ARGS__))

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
	BUFFEXT("#ifndef r_NormalScale\n#define r_NormalScale %f\n#endif\n", r_normalScale->value);

	BUFFEXT("#ifndef M_PI\n#define M_PI 3.14159265358979323846f\n#endif\n");
	BUFFEXT("#ifndef M_TAU\n#define M_TAU 6.28318530717958647693f\n#endif\n");
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
	        "#define GF_NONE %i\n"
	        "#define GF_SIN %i\n"
	        "#define GF_SQUARE %i\n"
	        "#define GF_TRIANGLE %i\n"
	        "#define GF_SAWTOOTH %i\n"
	        "#define GF_INVERSE_SAWTOOTH %i\n"
	        "#define GF_NOISE %i\n"
	        "#endif\n",
	        GF_NONE,
	        GF_SIN,
	        GF_SQUARE,
	        GF_TRIANGLE,
	        GF_SAWTOOTH,
	        GF_INVERSE_SAWTOOTH,
	        GF_NOISE);

	BUFFEXT("#ifndef deformGen_t\n"
	        "#define deformGen_t\n"
	        "#define DGEN_WAVE_SIN %i\n"
	        "#define DGEN_WAVE_SQUARE %i\n"
	        "#define DGEN_WAVE_TRIANGLE %i\n"
	        "#define DGEN_WAVE_SAWTOOTH %i\n"
	        "#define DGEN_WAVE_INVERSE_SAWTOOTH %i\n"
	        "#define DGEN_WAVE_NOISE %i\n"
	        "#define DGEN_BULGE %i\n"
	        "#define DGEN_MOVE %i\n"
	        "#endif\n",
	        DGEN_WAVE_SIN,
	        DGEN_WAVE_SQUARE,
	        DGEN_WAVE_TRIANGLE,
	        DGEN_WAVE_SAWTOOTH,
	        DGEN_WAVE_INVERSE_SAWTOOTH,
	        DGEN_WAVE_NOISE,
	        DGEN_BULGE,
	        DGEN_MOVE);


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

	BUFFEXT("#define ScreenWidth %i\n#define ScreenHeight %i\n", glConfig.vidWidth, glConfig.vidHeight);

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
			BUFFEXT("#ifndef r_EVSMExponents\n#define r_EVSMExponents vec2(%f, %f)\n#endif\n", 42.0, 42.0);

			if (r_evsmPostProcess->integer)
			{
				BUFFEXT("#ifndef r_EVSMPostProcess\n#define r_EVSMPostProcess 1\n#endif\n");
			}
		}
		else
		{
			BUFFEXT("#ifndef VSM\n#define VSM 1\n#endif\n");

			// FIXME: this was enabled for ati card.. Should not be needed anymore? Remove from GLSL code in that case
			//BUFFEXT("#ifndef VSM_CLAMP\n#define VSM_CLAMP 1\n#endif\n");
		}

		if (r_shadows->integer == SHADOWING_VSM32)
		{
			BUFFEXT("#ifndef VSM_EPSILON\n#define VSM_EPSILON 0.000001\n#endif\n");
		}
		else
		{
			BUFFEXT("#ifndef VSM_EPSILON\n#define VSM_EPSILON 0.0001\n#endif\n");
		}

		//if (r_lightBleedReduction->value)
		//{
		//	BUFFEXT("#ifndef r_LightBleedReduction\n#define r_LightBleedReduction %f\n#endif\n", r_lightBleedReduction->value);
		//}

		// exponential shadow mapping
		//if (r_overDarkeningFactor->value)
		//{
		//	BUFFEXT("#ifndef r_OverDarkeningFactor\n#define r_OverDarkeningFactor %f\n#endif\n", r_overDarkeningFactor->value);
		//}

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

	if (r_heatHazeFix->integer && glConfig2.framebufferBlitAvailable)
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

	if (r_wrapAroundLighting->integer)
	{
		BUFFEXT("#ifndef r_WrapAroundLighting\n#define r_WrapAroundLighting %i\n#endif\n", r_wrapAroundLighting->integer);
	}

	if (r_diffuseLighting->value >= 0.0) // && r_diffuseLighting->value <= 1.0
	{
		BUFFEXT("#ifndef r_diffuseLighting\n#define r_diffuseLighting %f\n#endif\n", r_diffuseLighting->value);
	}

	if (r_rimLighting->integer)
	{
		BUFFEXT("#ifndef r_rimLighting\n#define r_rimLighting 1\n#endif\n");
		BUFFEXT("#ifndef r_rimColor\n#define r_rimColor vec4(0.26, 0.19, 0.16, 0.0)\n#endif\n");
		BUFFEXT("#ifndef r_rimExponent\n#define r_rimExponent %f\n#endif\n", r_rimExponent->value);
	}

	// OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
	// so we have to reset the line counting
	BUFFEXT("#line 0\n");
	//shaderExtraDefLen = strlen(shaderExtraDef) + 1;
}

/**
 * @brief GLSL_GetShaderHeader
 * @param[in] shaderType
 * @param[out] dest
 * @param[in] size
 */
static void GLSL_GetShaderHeader(GLenum shaderType, char *dest, size_t size)
{
	dest[0] = '\0';

	Q_strcat(dest, size, "#version 330 core\n");

	if (shaderType == GL_VERTEX_SHADER)
	{
		Q_strcat(dest, size, "#define attribute in\n");
		Q_strcat(dest, size, "#define varying out\n");
	}
	else
	{
		Q_strcat(dest, size, "#define varying in\n");
		Q_strcat(dest, size, "out vec4 out_Color[4];\n");
		Q_strcat(dest, size, "#define gl_FragColor out_Color[0]\n");
		Q_strcat(dest, size, "#define gl_FragData out_Color\n");
	}

	Q_strcat(dest, size, "#define textureCube texture\n");
	Q_strcat(dest, size, "#define texture2D texture\n");
	Q_strcat(dest, size, "#define texture2DProj textureProj\n");
}

/**
 * @brief GLSL_CompileGPUShader
 * @param[in] program
 * @param[in,out] prevShader
 * @param[in] buffer
 * @param[in] size
 * @param[in] shaderType
 * @param[in] name
 * @return
 */
static int GLSL_CompileGPUShader(GLhandleARB program, GLhandleARB *prevShader, const GLcharARB *buffer, int size, GLenum shaderType, const char *name)
{
	GLint       compiled;
	GLhandleARB shader;

	shader = glCreateShader(shaderType);

	glShaderSource(shader, 1, (const GLcharARB **)&buffer, &size);

	// compile shader
	glCompileShader(shader);

	// check if shader compiled
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		const char *outPath = va("debug/%s_%s.debug", name, (shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment"));
		ri.FS_WriteFile(outPath, buffer, size);
		//GLSL_PrintShaderSource(shader);
		GLSL_PrintInfoLog(shader, qfalse, qfalse);
		Ren_Fatal("Couldn't compile shader \"%s\" wrote debug output to: %s", name, outPath);
		//return 0;
	}

	//GLSL_PrintInfoLog(shader, qtrue);
	//GLSL_PrintShaderSource(shader);

	if (*prevShader)
	{
		glDetachShader(program, *prevShader);
		glDeleteShader(*prevShader);
	}

	// attach shader to program
	glAttachShader(program, shader);

	*prevShader = shader;

	return 1;
}

/**
 * @brief GLSL_GetShaderText loads shader text into data buffer
 * @param[in] name
 * @param[in] shaderType
 * @param[out] data
 * @param[out] size
 * @param[in] append
 *
 * @note We don't ship glsl files anymore - GetFallbackShader() does the job
 */
static void GLSL_GetShaderText(const char *name, GLenum shaderType, char **data, size_t *size, qboolean append)
{
	char   fullname[MAX_QPATH];
	size_t dataSize    = 0;
	char   *dataBuffer = NULL;

	if (shaderType == GL_VERTEX_SHADER)
	{
		Com_sprintf(fullname, sizeof(fullname), "%s_vp", name);
		Ren_Developer("...loading vertex shader '%s'\n", fullname);
	}
	else
	{
		Com_sprintf(fullname, sizeof(fullname), "%s_fp", name);
		Ren_Developer("...loading other/fragment shader '%s'\n", fullname);
	}

	if (ri.FS_FOpenFileRead(va("glsl/%s.glsl", fullname), NULL, qfalse) > 0)
	{
		dataSize = ri.FS_ReadFile(va("glsl/%s.glsl", fullname), ( void ** ) &dataBuffer);
	}

	if (!dataBuffer)
	{
		const char *temp;

		temp = GetFallbackShader(fullname);
		if (temp)
		{
			// Found a fallback shader and will use it
			size_t strl = 0;
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
			Ren_Fatal("Couldn't load shader %s", fullname);
		}
	}
	else
	{
		++dataSize; // We increase this for the newline
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

		ri.FS_FreeFile(dataBuffer);
	}

	Ren_Developer("Loaded shader '%s'\n", fullname);
}

/**
 * @brief GLSL_PreprocessShaderText
 * @param[in] shaderBuffer
 * @param[in,out] filetext
 * @param[in] shadertype
 */
static void GLSL_PreprocessShaderText(char *shaderBuffer, char *filetext, GLenum shadertype)
{
	GLchar *ref   = filetext;
	char   *token = NULL;
	int    c      = 0;
	size_t offset = 0;

	while ((c = *ref))
	{
		// skip double slash comments
		if (c == '/' && ref[1] == '/')
		{
			ref += 2;
			while (*ref && *ref != '\n')
			{
				ref++;
			}
		}
		// skip /* */ comments
		else if (c == '/' && ref[1] == '*')
		{
			ref += 2;
			while (*ref && (*ref != '*' || ref[1] != '/'))
			{
				ref++;
			}
			if (*ref)
			{
				ref += 2;
			}
		}
		// We found a # command
		else if (c == '#')
		{
			char *ref2 = ref;

			ref++;
			token = COM_ParseExt2(&ref, qfalse);

			if (!Q_stricmp(token, "include"))
			{
				// handle include
				GLchar *libBuffer         = NULL;
				size_t libBufferSize      = 0;
				size_t currentOffset      = strlen(shaderBuffer);
				char   *shaderBufferPoint = NULL;

				if (!currentOffset)
				{
					shaderBufferPoint = shaderBuffer;
				}
				else
				{
					shaderBufferPoint = &shaderBuffer[currentOffset];
				}

				token = COM_ParseExt2(&ref, qfalse);
				GLSL_GetShaderText(token, shadertype, &libBuffer, &libBufferSize, qfalse);
				GLSL_PreprocessShaderText(shaderBufferPoint, libBuffer, shadertype);
				//Q_strcat(shaderBuffer, libBufferSize, libBuffer);
				Com_Dealloc(libBuffer);
				token = NULL;
			}
			else
			{
				ref                  = ref2;
				shaderBuffer[offset] = c;
			}

			offset = strlen(shaderBuffer);
		}
		// Just add the char to the buffer
		else
		{
			shaderBuffer[offset] = c;
			offset++;
		}

		ref++;
	}
}

#define GLSL_BUFF 64000
#define GLSL_BUFF_CHAR (sizeof(char) *GLSL_BUFF)

/**
 * @brief GLSL_BuildGPUShaderText
 * @param[in] info
 * @param[in] shadertype
 * @return
 */
static char *GLSL_BuildGPUShaderText(programInfo_t *info, GLenum shadertype)
{
	static char shaderBuffer[GLSL_BUFF];
	GLchar      *mainBuffer    = NULL;
	size_t      mainBufferSize = 0;
	char        *filename      = NULL;
	char        *output        = NULL;

	GL_CheckErrors();

	Com_Memset(shaderBuffer, '\0', GLSL_BUFF_CHAR);

	switch (shadertype)
	{
	case GL_VERTEX_SHADER:
		filename = info->filename;
		break;
	case GL_FRAGMENT_SHADER:
		filename = (info->fragFilename ? info->fragFilename : info->filename);
		break;
	case GL_GEOMETRY_SHADER:
	// TODO: handle
	//break;
	case GL_TESS_CONTROL_SHADER:
	// TODO: handle
	//break;
	case GL_TESS_EVALUATION_SHADER:
	// TODO: handle
	//break;
	default:
		Ren_Fatal("WTF");
		return NULL;
	}

	strcpy(shaderBuffer, shaderExtraDef);

	GLSL_GetShaderText(filename, shadertype, &mainBuffer, &mainBufferSize, qfalse);
	GLSL_PreprocessShaderText(&shaderBuffer[strlen(shaderBuffer)], mainBuffer, shadertype);

	Com_Dealloc(mainBuffer);

	output = Com_Allocate(strlen(shaderBuffer) * sizeof(char) + 1);
	strcpy(output, shaderBuffer);
	return output;
}

/**
 * @brief GLSL_GenerateMacroString
 * @param[in] program
 * @param[in] macros
 * @param[in] permutation
 * @param[out] out
 * @return
 *
 * @warning This whole method is stupid, clean this shit up
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
					Ren_Print("GLSL_GenerateMacroString Info: Conflicting macros found\n");
					return qfalse;
				}

				if (GLSL_MissesRequiredMacros(i, macroatrib))
				{
					Ren_Print("GLSL_GenerateMacroString Info: Missing required macros program '%s' macros: '%s' macroattrib: %i\n", complieMacroNames[i], macros, macroatrib);
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

/**
 * @brief GLSL_LinkProgram
 * @param[in] program
 */
static void GLSL_LinkProgram(GLhandleARB program)
{
	GLint linked;

	// Apparently, this is necessary to get the binary program via glGetProgramBinary
	if (glConfig2.getProgramBinaryAvailable)
	{
		glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
	}

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		GLSL_PrintInfoLog(program, qfalse, qtrue);
		Ren_Print("\n");
		Ren_Drop("shaders failed to link");
	}
}

/**
 * @brief GLSL_ValidateProgram
 * @param[in] program
 */
static void GLSL_ValidateProgram(GLhandleARB program)
{
	GLint validated;

	glValidateProgram(program);

	glGetProgramiv(program, GL_VALIDATE_STATUS, &validated);
	if (!validated)
	{
		GLSL_PrintInfoLog(program, qfalse, qtrue);
		Ren_Print("\n");
		Ren_Drop("shaders failed to validate");
	}
}

/**
 * @brief GLSL_ShowProgramUniforms
 * @param[in] program
 */
static void GLSL_ShowProgramUniforms(GLhandleARB program)
{
	int    i, count, size;
	GLenum type;
	char   uniformName[1000];

	// install the executables in the program object as part of current state.
	glUseProgram(program);

	// check for GL Errors

	// query the number of active uniforms
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);

	// Loop over each of the active uniforms, and set their value
	for (i = 0; i < count; i++)
	{
		glGetActiveUniform(program, i, sizeof(uniformName), NULL, &size, &type, uniformName);

		Ren_LogComment("active uniform: '%s'\n", uniformName);
	}

	glUseProgram(0);
}

/**
 * @brief GLSL_InitUniforms
 * @param[out] program
 */
void GLSL_InitUniforms(shaderProgram_t *program)
{
	int   i, size = 0;
	GLint *uniforms = program->uniforms;

	for (i = 0; i < UNIFORM_COUNT; i++)
	{
		uniforms[i] = glGetUniformLocation(program->program, uniformsInfo[i].name);

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
		case GLSL_DOUBLE:
			size += sizeof(GLdouble);
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

/**
 * @brief GLSL_FinishGPUShader
 * @param[in] program
 */
void GLSL_FinishGPUShader(shaderProgram_t *program)
{
	GLSL_ValidateProgram(program->program);
	GLSL_ShowProgramUniforms(program->program);
	GL_CheckErrors();
}

/**
 * @brief GLSL_SelectTexture
 * @param[in] program
 * @param[in] tex
 */
void GLSL_SelectTexture(shaderProgram_t *program, texture_def_t tex)
{
	if (program->textureBinds[tex] == -1)
	{
		//Ren_Fatal("GLSL_SelectTexture: Trying to select non existing texture %i %s\n", tex, program->name);
		Ren_Warning("GLSL_SelectTexture: Trying to select non existing texture %i - program name:'%s'\n", tex, program->name);

		GL_SelectTexture(0);
		return;
	}

	GL_SelectTexture(program->textureBinds[tex]);
}

/**
 * @brief GLSL_SetUniformBoolean
 * @param[in] program
 * @param[in] uniformNum
 * @param[in] value
 */
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
		Ren_Fatal("GLSL_SetUniformBoolean: wrong type for uniform %i in program %s\n", uniformNum, program->name);
	}

	if (value == *compare)
	{
		return;
	}

	*compare = value;

	glUniform1i(uniforms[uniformNum], value);
}

/**
 * @brief GLSL_SetUniformInt
 * @param[in] program
 * @param[in] uniformNum
 * @param[in] value
 */
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
		Ren_Fatal("GLSL_SetUniformInt: wrong type for uniform %i in program %s\n", uniformNum, program->name);
	}

	if (value == *compare)
	{
		return;
	}

	*compare = value;

	glUniform1i(uniforms[uniformNum], value);
}

/**
 * @brief GLSL_SetUniformFloat
 * @param[in] program
 * @param[in] uniformNum
 * @param[in] value
 */
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
		Ren_Fatal("GLSL_SetUniformFloat: wrong type for uniform %i in program %s\n", uniformNum, program->name);
	}

	if (value == *compare)
	{
		return;
	}

	*compare = value;

	glUniform1f(uniforms[uniformNum], value);
}

/**
 * @brief GLSL_SetUniformDouble
 * @param[in] program
 * @param[in] uniformNum
 * @param[in] value
 */
void GLSL_SetUniformDouble(shaderProgram_t *program, int uniformNum, GLdouble value)
{
	GLint    *uniforms = program->uniforms;
	GLdouble *compare  = (GLdouble *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_DOUBLE)
	{
		Ren_Fatal("GLSL_SetUniformDouble: wrong type for uniform %i in program %s\n", uniformNum, program->name);
	}

	if (value == *compare)
	{
		return;
	}

	*compare = value;

	glUniform1f(uniforms[uniformNum], value);
}


/**
 * @brief GLSL_SetUniformVec2
 * @param[in] program
 * @param[in] uniformNum
 * @param[in] v
 */
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
		Ren_Fatal("GLSL_SetUniformVec2: wrong type for uniform %i in program %s\n", uniformNum, program->name);
	}

	if (v[0] == compare[0] && v[1] == compare[1])
	{
		return;
	}

	compare[0] = v[0];
	compare[1] = v[1];

	glUniform2f(uniforms[uniformNum], v[0], v[1]);
}

/**
 * @brief GLSL_SetUniformVec3
 * @param[in] program
 * @param[in] uniformNum
 * @param[in] v
 */
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
		Ren_Fatal("GLSL_SetUniformVec3: wrong type for uniform %i in program %s\n", uniformNum, program->name);
	}

	if (VectorCompare(v, compare))
	{
		return;
	}

	VectorCopy(v, compare);

	glUniform3f(uniforms[uniformNum], v[0], v[1], v[2]);
}

/**
 * @brief GLSL_SetUniformVec4
 * @param[in] program
 * @param[in] uniformNum
 * @param[in] v
 */
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
		Ren_Fatal("GLSL_SetUniformVec4: wrong type for uniform %i in program %s\n", uniformNum, program->name);
	}

	if (Vector4Compare(v, compare))
	{
		return;
	}

	Vector4Copy(v, compare);

	glUniform4f(uniforms[uniformNum], v[0], v[1], v[2], v[3]);
}

/**
 * @brief GLSL_SetUniformFloat5
 * @param[in] program
 * @param[in] uniformNum
 * @param[in] v
 */
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
		Ren_Fatal("GLSL_SetUniformFloat5: wrong type for uniform %i in program %s\n", uniformNum, program->name);
	}

	if (Vector5Compare(v, compare))
	{
		return;
	}

	Vector5Copy(v, compare);

	glUniform1fv(uniforms[uniformNum], 5, v);
}

/**
 * @brief GLSL_SetUniformMatrix16
 * @param[in] program
 * @param[in] uniformNum
 * @param[in] matrix
 */
void GLSL_SetUniformMatrix16(shaderProgram_t *program, int uniformNum, const mat4_t matrix)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare  = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_MAT16)
	{
		Ren_Fatal("GLSL_SetUniformMatrix16: wrong type for uniform %i in program %s\n", uniformNum, program->name);
	}

	if (mat4_compare(matrix, compare))
	{
		return;
	}

	mat4_copy(matrix, compare);

	glUniformMatrix4fv(uniforms[uniformNum], 1, GL_FALSE, matrix);
}

/**
 * @brief GLSL_SetUniformFloatARR
 * @param[in] program
 * @param[in] uniformNum
 * @param[in] floatarray
 * @param[in] arraysize
 */
void GLSL_SetUniformFloatARR(shaderProgram_t *program, int uniformNum, float *floatarray, int arraysize)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_FLOATARR)
	{
		Ren_Fatal("GLSL_SetUniformFloatARR: wrong type for uniform %i in program %s\n", uniformNum, program->name);
	}

	glUniform1fv(uniforms[uniformNum], arraysize, floatarray);
}

/**
 * @brief GLSL_SetUniformVec4ARR
 * @param[in] program
 * @param[in] uniformNum
 * @param[in] vectorarray
 * @param[in] arraysize
 */
void GLSL_SetUniformVec4ARR(shaderProgram_t *program, int uniformNum, vec4_t *vectorarray, int arraysize)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_VEC4ARR)
	{
		Ren_Fatal("GLSL_SetUniformVec4ARR: wrong type for uniform %i in program %s\n", uniformNum, program->name);
	}

	glUniform4fv(uniforms[uniformNum], arraysize, &vectorarray[0][0]);
}

/**
 * @brief GLSL_SetUniformMatrix16ARR
 * @param[in] program
 * @param[in] uniformNum
 * @param[in] matrixarray
 * @param[in] arraysize
 */
void GLSL_SetUniformMatrix16ARR(shaderProgram_t *program, int uniformNum, mat4_t *matrixarray, int arraysize)
{
	GLint *uniforms = program->uniforms;

	if (uniforms[uniformNum] == -1)
	{
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_MAT16ARR)
	{
		Ren_Fatal("GLSL_SetUniformMatrix16ARR: wrong type for uniform %s in program %s\n", uniformsInfo[uniformNum].name, program->name);
	}

	glUniformMatrix4fv(uniforms[uniformNum], arraysize, GL_FALSE, &matrixarray[0][0]);
}

/**
 * @brief GLSL_BindAttribLocations
 * @param[in] program
 */
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

/**
 * @brief GLSL_InitGPUShader2
 * @param[in,out] program
 * @param[in] name
 * @param[in] vpCode
 * @param[in] fpCode
 * @return
 */
static qboolean GLSL_InitGPUShader2(shaderProgram_t *program, const char *name, const char *vpCode, const char *fpCode)
{
	Ren_LogComment("------- GLSL_InitGPUShader2 -------\n");

	if (strlen(name) >= MAX_QPATH)
	{
		Ren_Drop("GLSL_InitGPUShader2: \"%s\" is too long", name);
	}

	Q_strncpyz(program->name, name, sizeof(program->name));

	program->program = glCreateProgram();

	if (!(GLSL_CompileGPUShader(program->program, &program->vertexShader, vpCode, strlen(vpCode), GL_VERTEX_SHADER, name)))
	{
		Ren_Print("GLSL_InitGPUShader2: Unable to load \"%s\" as GL_VERTEX_SHADER_ARB\n", name);
		glDeleteProgram(program->program);
		return qfalse;
	}

	if (fpCode)
	{
		if (!(GLSL_CompileGPUShader(program->program, &program->fragmentShader, fpCode, strlen(fpCode), GL_FRAGMENT_SHADER, name)))
		{
			Ren_Print("GLSL_InitGPUShader2: Unable to load \"%s\" as GL_FRAGMENT_SHADER_ARB\n", name);
			glDeleteProgram(program->program);
			return qfalse;
		}
	}

	GLSL_BindAttribLocations(program->program);

	GLSL_LinkProgram(program->program);

	return qtrue;
}

/**
 * @brief GLSL_FinnishShaderTextAndCompile
 * @param[in] info
 * @param[in] permutation
 * @param[in] vertex
 * @param[in] frag
 * @param[in] macrostring
 * @return
 */
static qboolean GLSL_FinnishShaderTextAndCompile(programInfo_t *info, int permutation, const char *vertex, const char *frag, const char *macrostring)
{
	char   vpSource[64000];
	char   fpSource[64000];
	size_t size = sizeof(vpSource);

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

	return qfalse;
}

/**
 * @brief GLSL_GetProgramPermutation
 * @param[in] info
 * @param[in] permutation
 * @param[in] vertex
 * @param[in] frag
 * @param[in] macrostring
 * @return
 */
static qboolean GLSL_GetProgramPermutation(programInfo_t *info, int permutation, const char *vertex, const char *frag, const char *macrostring)
{
	// load bin
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

/**
 * @brief GLSL_SetTextureUnitBindings
 * @param[in] info
 * @param[in] permutation
 */
static void GLSL_SetTextureUnitBindings(programInfo_t *info, int permutation)
{
	int             i, j;
	shaderProgram_t *program = &info->list->programs[permutation];

	for (i = 0, j = 0; i < TEX_COUNT; i++)
	{
		if (program->uniforms[textureMap[i]] == -1)
		{
			program->textureBinds[i] = -1;
		}
		else
		{
			program->textureBinds[i] = j;
			glUniform1i(program->uniforms[textureMap[i]], j);
			j++;
		}
	}
}

/**
 * @brief GLSL_SetInitialUniformValues
 * @param[in] info
 * @param[in] permutation
 */
static void GLSL_SetInitialUniformValues(programInfo_t *info, int permutation)
{
	int i, location;

	for (i = 0; i < info->numUniformValues; i++)
	{
		location = glGetUniformLocation(info->list->programs[permutation].program, info->uniformValues[i].type.name);

		if (location == -1)
		{
			Ren_Warning("Cannot find uniform \"%s\" from program: %s %d\n", info->uniformValues[i].type.name, info->name, location);
			Ren_LogComment("Cannot find uniform \"%s\" from program: %s %d\n", info->uniformValues[i].type.name, info->name, location);

			continue;
		}

		switch (info->uniformValues[i].type.type)
		{
		case GLSL_BOOL:
			GLSL_SetUniformBoolean(&info->list->programs[permutation], location, *((GLboolean *)info->uniformValues[i].value));
			break;
		case GLSL_INT:
			GLSL_SetUniformInt(&info->list->programs[permutation], location, *((GLint *)info->uniformValues[i].value));
			break;
		case GLSL_FLOAT:
			GLSL_SetUniformFloat(&info->list->programs[permutation], location, *((GLfloat *)info->uniformValues[i].value));
			break;
		case GLSL_FLOAT5:
			GLSL_SetUniformFloat5(&info->list->programs[permutation], location, *((vec5_t *)info->uniformValues[i].value));
			break;
		case GLSL_DOUBLE:
			GLSL_SetUniformDouble(&info->list->programs[permutation], location, *((GLdouble *)info->uniformValues[i].value));
			break;
		case GLSL_VEC2:
			GLSL_SetUniformVec2(&info->list->programs[permutation], location, *((vec2_t *)info->uniformValues[i].value));
			break;
		case GLSL_VEC3:
			GLSL_SetUniformVec3(&info->list->programs[permutation], location, *((vec3_t *)info->uniformValues[i].value));
			break;
		case GLSL_VEC4:
			GLSL_SetUniformVec4(&info->list->programs[permutation], location, *((vec4_t *)info->uniformValues[i].value));
			break;
		/*	FIXME:
		        case GLSL_MAT16:
		            GLSL_SetUniformMatrix16(&info->list->programs[permutation],location,**((mat4_t *)info->uniformValues[i].value));
		            break;
		        case GLSL_FLOATARR:
		            GLSL_SetUniformFloatARR(&info->list->programs[permutation],location,**((float *)info->uniformValues[i].value));
		            break;
		        case GLSL_VEC4ARR:
		            GLSL_SetUniformVec4ARR(&info->list->programs[permutation],location,**((vec4_t *)info->uniformValues[i].value));
		            break;
		        case GLSL_MAT16ARR:
		            GLSL_SetUniformMatrix16ARR(&info->list->programs[permutation],location,**((mat4_t *)info->uniformValues[i].value));
		            break;
		*/
		default:
			Ren_Fatal("Only INT supported atm");
		}

		Ren_LogComment("Setting initial uniform \"%s\" value: %d\n", info->uniformValues[i].type.name, *((int *)info->uniformValues[i].value));
	}
}

/**
 * @brief GLSL_GenerateCheckSum
 * @param[out] info
 * @param[in] vertex
 * @param[in] fragment
 */
void GLSL_GenerateCheckSum(programInfo_t *info, const char *vertex, const char *fragment)
{
	char   *fullSource;
	size_t size = 0;

	size += strlen(vertex);
	size += strlen(fragment);
	size *= sizeof(char);
	size += 1;

	fullSource = (char *)ri.Hunk_AllocateTempMemory(size);
	if (!fullSource)
	{
		Ren_Fatal("Failed to allocate memory for checksum string\n");
	}

	Com_Memset(fullSource, '\0', size);
	Q_strcat(fullSource, size, vertex);
	Q_strcat(fullSource, size, fragment);

	info->checkSum = Com_BlockChecksum(fullSource, strlen(fullSource));

	ri.Hunk_FreeTempMemory(fullSource);
}

/**
 * @brief GLSL_CompilePermutation
 * @param[in,out] info
 * @param[in] offset
 * @return
 */
static qboolean GLSL_CompilePermutation(programInfo_t *info, int offset)
{
	char     *tempString = NULL;
	qboolean compiled    = qfalse;

	if (GLSL_GenerateMacroString(info->list, info->extraMacros, offset, &tempString))
	{
		if (GLSL_GetProgramPermutation(info, offset, info->vertexShaderText, info->fragmentShaderText, tempString))
		{
			GLSL_BindProgram(&info->list->programs[offset]);
			// Set uniform values
			GLSL_SetTextureUnitBindings(info, offset);
			GLSL_SetInitialUniformValues(info, offset);
			GLSL_BindNullProgram();

			GLSL_FinishGPUShader(&info->list->programs[offset]);
			info->list->programs[offset].compiled = qtrue;
		}
		else
		{
			Ren_Fatal("Failed to compile shader: %s permutation %d\n", info->name, offset);
		}

		compiled = qtrue;
	}
	else
	{
		info->list->programs[offset].program  = 0;
		info->list->programs[offset].compiled = qfalse;
	}

	Com_Dealloc(tempString); // see GLSL_GenerateMacroString
	return compiled;
}

/**
 * @brief GLSL_CompileShaderProgram
 * @param[in,out] info
 * @return
 */
qboolean GLSL_CompileShaderProgram(programInfo_t *info)
{
	info->vertexShaderText   = GLSL_BuildGPUShaderText(info, GL_VERTEX_SHADER);
	info->fragmentShaderText = GLSL_BuildGPUShaderText(info, GL_FRAGMENT_SHADER);
#if GLSL_PRECOMPILE
	int    startTime, endTime, nextTicCount = 0;
	size_t numCompiled = 0, tics = 0;
#endif
	size_t numPermutations = 0;
	int    i = 0, x = 0;

	GLSL_GenerateCheckSum(info, info->vertexShaderText, info->fragmentShaderText);

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

	info->list->programs = (shaderProgram_t *)Com_Allocate(sizeof(shaderProgram_t) * numPermutations);
	Com_Memset(info->list->programs, 0, sizeof(shaderProgram_t) * numPermutations);

#if GLSL_PRECOMPILE
	Ren_Print("...compiling %s shaders\n", info->name);
	Ren_Print("0%%  10   20   30   40   50   60   70   80   90   100%%\n");
	Ren_Print("|----|----|----|----|----|----|----|----|----|----|\n");

	startTime = ri.Milliseconds();

	for (i = 0; i < numPermutations; i++)
	{
		if ((i + 1) >= nextTicCount)
		{
			size_t ticsNeeded = (size_t)(((double)(i + 1) / numPermutations) * 50.0);

			do
			{
				Ren_Print("*");
			}
			while (++tics < ticsNeeded);

			nextTicCount = (size_t)((tics / 50.0) * numPermutations);

			if (i == (numPermutations - 1))
			{
				if (tics < 51)
				{
					Ren_Print("*");
				}

				Ren_Print("\n");
			}
		}

		if (GLSL_CompilePermutation(info, i))
		{
			numCompiled++;
		}
	}

	endTime = ri.Milliseconds();
	Ren_Print("...compiled %i %s shader permutations in %5.2f seconds\n", ( int ) numCompiled, info->name, (endTime - startTime) / 1000.0);
#endif

	info->compiled                 = qtrue;
	info->list->currentPermutation = 0;

	return qtrue;
}

/**
 * @brief GLSL_GetShaderProgram
 * @param[in] name
 * @return
 */
programInfo_t *GLSL_GetShaderProgram(const char *name)
{
	programInfo_t *prog;

	prog = GLSL_FindShader(name);

	if (prog && !prog->compiled)
	{
		// Compile the shader program
		GLSL_CompileShaderProgram(prog);
	}

	return prog;
}

/**
 * @brief GLSL_SetMacroStatesByOffset
 * @param[in,out] programlist
 * @param[in] offset
 */
void GLSL_SetMacroStatesByOffset(programInfo_t *programlist, int offset)
{
	if (offset < 0)
	{
		Ren_Fatal("Trying to select an negative array cell\n");
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

/**
 * @brief GLSL_SetMacroState
 * @param[in,out] programlist
 * @param[in] macro
 * @param[in] enabled
 */
void GLSL_SetMacroState(programInfo_t *programlist, int macro, int enabled)
{
	if (!programlist)
	{
		Ren_Fatal("GLSL_SetMacroState: NULL programinfo");
	}

	if (!programlist->compiled)
	{
		Ren_Fatal("Trying to set macro state of shader \"%s\" but it is not compiled\n", programlist->name);
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
		Ren_Fatal("GLSL_SetMacroState: Trying to set macro state to impossible result for shader: %s with macro: %i permutation number %zu", programlist->name, macro, programlist->list->permutations);
	}
}

/**
 * @brief GLSL_SetMacroStates
 * @param[in] programlist
 * @param[in] numMacros
 */
void GLSL_SetMacroStates(programInfo_t *programlist, int numMacros, ...)
{
	int     macro, value;
	va_list ap;

	// Reset the macro states
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
		va_end(ap);
		return;
	}

	if (numMacros % 2 != 0)
	{
		Ren_Fatal("GLSL_SetMacroStates: Trying to set macros with an array which has an invalid size %i\n", numMacros);
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

/**
 * @brief GLSL_SelectPermutation
 * @param[in] programlist
 */
void GLSL_SelectPermutation(programInfo_t *programlist)
{
	shaderProgram_t *prog;

	if (!programlist)
	{
		Ren_Fatal("GLSL_SelectPermutation: NULL programinfo");
	}

	if (!programlist->compiled)
	{
		Ren_Fatal("Trying to select permutation of shader \"%s\" but the list is not compiled\n", programlist->name);
	}

	prog = &programlist->list->programs[programlist->list->currentPermutation];

	if (!prog || !prog->compiled)
	{
#ifndef GLSL_PRECOMPILE
		if (!GLSL_CompilePermutation(programlist, programlist->list->currentPermutation))
		{
			Ren_Fatal("Trying to select uncompileable shader permutation: %d of shader \"%s\"\n", programlist->list->currentPermutation, programlist->name);
		}
		else
		{
			trProg.selectedProgram = programlist->list->current = prog;
			GLSL_BindProgram(prog);
		}
#else

		Ren_Fatal("Trying to select uncompiled shader permutation: %d of shader \"%s\"\n", programlist->list->currentPermutation, programlist->name);
#endif
	}
	else
	{
		trProg.selectedProgram = programlist->list->current = prog;
		GLSL_BindProgram(prog);
	}
}

/**
 * @brief GLSL_SetRequiredVertexPointers
 * @param[in] programlist
 */
void GLSL_SetRequiredVertexPointers(programInfo_t *programlist)
{
	// FIXME: implement this
	// see void GLShader::SetRequiredVertexPointers() in gl_shader.cpp

	//uint32_t macroVertexAttribs = 0;
	//size_t   numMacros          = _compileMacros.size();

	//for (size_t j = 0; j < numMacros; j++)
	//{
	//    GLCompileMacro *macro = _compileMacros[j];

	//    int bit = macro->GetBit();

	//    if (IsMacroSet(bit))
	//    {
	//        macroVertexAttribs |= macro->GetRequiredVertexAttributes();
	//    }
	//}

	//GLSL_VertexAttribsState((_vertexAttribsRequired | _vertexAttribs | macroVertexAttribs));      // & ~_vertexAttribsUnsupported);

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

/**
 * @brief GLSL_DeleteGPUShader
 * @param[in] program
 */
void GLSL_DeleteGPUShader(shaderProgram_t *program)
{
	if (program->program)
	{
		if (program->vertexShader)
		{
			glDetachShader(program->program, program->vertexShader);
			glDeleteShader(program->vertexShader);
		}

		if (program->fragmentShader)
		{
			glDetachShader(program->program, program->fragmentShader);
			glDeleteShader(program->fragmentShader);
		}

		glDeleteProgram(program->program);

		if (program->uniformBuffer)
		{
			Com_Dealloc(program->uniformBuffer);
		}

		Com_Memset(program, 0, sizeof(*program));
	}
}

/**
 * @brief GLSL_DeleteShaderProgramList
 * @param[in] programlist
 */
void GLSL_DeleteShaderProgramList(shaderProgramList_t *programlist)
{
	int i;

	for (i = 0; i < programlist->permutations; i++)
	{
		GLSL_DeleteGPUShader(&programlist->programs[i]);
	}

	Com_Dealloc(programlist->programs);
}

/**
 * @brief GLSL_DeleteShaderProgramInfo
 * @param[in] program
 */
void GLSL_DeleteShaderProgramInfo(programInfo_t *program)
{
	int i;

	if (program->list)
	{
		GLSL_DeleteShaderProgramList(program->list);
		Com_Dealloc(program->list);
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

	if (program->vertexShaderText)
	{
		Com_Dealloc(program->vertexShaderText);
		program->vertexShaderText = NULL;
	}

	if (program->fragmentShaderText)
	{
		Com_Dealloc(program->fragmentShaderText);
		program->fragmentShaderText = NULL;
	}

	for (i = 0; i < program->numUniformValues; i++)
	{
		if (program->uniformValues[i].type.name)
		{
			Com_Dealloc(program->uniformValues[i].type.name);
		}

		if (program->uniformValues[i].value)
		{
			Com_Dealloc(program->uniformValues[i].value);
		}
	}
}

/**
 * @brief GLSL_InitGPUShaders
 */
void GLSL_InitGPUShaders(void)
{
#ifdef ETLEGACY_DEBUG
	int startTime, endTime;
#endif

	Ren_LogComment("------- GLSL_InitGPUShaders -------\n");

	R_IssuePendingRenderCommands();

#ifdef ETLEGACY_DEBUG
	startTime = ri.Milliseconds();
#endif

	// Load all definitions
	GLSL_LoadDefinitions();
	GLSL_BuildShaderExtraDef();

#ifdef ETLEGACY_DEBUG
	endTime = ri.Milliseconds();

	Ren_Developer("Initialized GLSL system in %5.2f seconds\n", (endTime - startTime) / 1000.0);
#endif
}

/**
 * @brief GLSL_CompileGPUShaders
 */
void GLSL_CompileGPUShaders(void)
{
#ifdef ETLEGACY_DEBUG
	int startTime, endTime;
#endif

	Ren_LogComment("------- GLSL_CompileGPUShaders -------\n");

	R_IssuePendingRenderCommands();

	//R_BindFBO(tr.deferredRenderFBO);

	// Init simple shader and draw loading screen
#ifdef ETLEGACY_DEBUG
	startTime = ri.Milliseconds();
#endif

	Com_Memset(&trProg, 0, sizeof(trPrograms_t));

	trProg.gl_genericShader      = GLSL_GetShaderProgram("generic");
	trProg.gl_lightMappingShader = GLSL_GetShaderProgram("lightMapping");

	trProg.gl_vertexLightingShader_DBS_entity = GLSL_GetShaderProgram("vertexLighting_DBS_entity");
	trProg.gl_vertexLightingShader_DBS_world  = GLSL_GetShaderProgram("vertexLighting_DBS_world");

	trProg.gl_forwardLightingShader_omniXYZ        = GLSL_GetShaderProgram("forwardLighting_omniXYZ");
	trProg.gl_forwardLightingShader_projXYZ        = GLSL_GetShaderProgram("forwardLighting_projXYZ");
	trProg.gl_forwardLightingShader_directionalSun = GLSL_GetShaderProgram("forwardLighting_directionalSun");

	trProg.gl_shadowFillShader     = GLSL_GetShaderProgram("shadowFill");
	trProg.gl_reflectionShader     = GLSL_GetShaderProgram("reflection");
	trProg.gl_skyboxShader         = GLSL_GetShaderProgram("skybox");
	trProg.gl_fogQuake3Shader      = GLSL_GetShaderProgram("fogQuake3");
	trProg.gl_fogGlobalShader      = GLSL_GetShaderProgram("fogGlobal");
	trProg.gl_heatHazeShader       = GLSL_GetShaderProgram("heatHaze");
	trProg.gl_screenShader         = GLSL_GetShaderProgram("screen");
	trProg.gl_portalShader         = GLSL_GetShaderProgram("portal");
	trProg.gl_toneMappingShader    = GLSL_GetShaderProgram("toneMapping");
	trProg.gl_contrastShader       = GLSL_GetShaderProgram("contrast");
	trProg.gl_cameraEffectsShader  = GLSL_GetShaderProgram("cameraEffects");
	trProg.gl_blurXShader          = GLSL_GetShaderProgram("blurX");
	trProg.gl_blurYShader          = GLSL_GetShaderProgram("blurY");
	trProg.gl_debugShadowMapShader = GLSL_GetShaderProgram("debugShadowMap");

	trProg.gl_liquidShader             = GLSL_GetShaderProgram("liquid");
	trProg.gl_rotoscopeShader          = GLSL_GetShaderProgram("rotoscope");
	trProg.gl_bloomShader              = GLSL_GetShaderProgram("bloom");
	trProg.gl_refractionShader         = GLSL_GetShaderProgram("refraction");
	trProg.gl_depthToColorShader       = GLSL_GetShaderProgram("depthToColor");
	trProg.gl_volumetricFogShader      = GLSL_GetShaderProgram("volumetricFog");
	trProg.gl_volumetricLightingShader = GLSL_GetShaderProgram("lightVolume_omni");
	trProg.gl_dispersionShader         = GLSL_GetShaderProgram("dispersion");

	trProg.gl_depthOfField = GLSL_GetShaderProgram("depthOfField");
	trProg.gl_ssao         = GLSL_GetShaderProgram("SSAO");

	trProg.gl_colorCorrection = GLSL_GetShaderProgram("colorCorrection");

#ifdef ETLEGACY_DEBUG
	endTime = ri.Milliseconds();

	Ren_Developer("Compiled default shader programs in %5.2f seconds\n", (endTime - startTime) / 1000.0);
#endif

	if (r_recompileShaders->integer)
	{
		ri.Cvar_Set("r_recompileShaders", "0");
	}
}

/**
 * @brief GLSL_ShutdownGPUShaders
 */
void GLSL_ShutdownGPUShaders(void)
{
	int i;

	Ren_LogComment("------- GLSL_ShutdownGPUShaders -------\n");

	for (i = 0; i < ATTR_INDEX_COUNT; i++)
	{
		glDisableVertexAttribArray(i);
	}

	GLSL_BindNullProgram();

	// Clean up programInfo_t:s
	for (i = 0; i < FILE_HASH_SIZE; i++)
	{
		if (hashTable[i])
		{
			programInfo_t *prog = hashTable[i];
			GLSL_DeleteShaderProgramInfo(prog);
			Com_Dealloc(prog);
			hashTable[i] = NULL;
		}
	}

	Com_Dealloc(definitionText);

	Com_Memset(&trProg, 0, sizeof(trPrograms_t));
	glUseProgram(0);
}

/**
 * @brief GLSL_BindProgram
 * @param[in] program
 */
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
		glUseProgram(program->program);
		glState.currentProgram = program;
		backEnd.pc.c_glslShaderBinds++;
	}
}

/**
 * @brief GLSL_BindNullProgram
 */
void GLSL_BindNullProgram(void)
{
	Ren_LogComment("--- GL_BindNullProgram ---\n");

	if (glState.currentProgram)
	{
		glUseProgram(0);
		glState.currentProgram = NULL;
	}
}

/**
 * @brief GLSL_SetUniform_DeformParms
 * @param[in] deforms
 * @param[in] numDeforms
 */
void GLSL_SetUniform_DeformParms(deformStage_t deforms[], int numDeforms)
{
	float deformParms[MAX_SHADER_DEFORM_PARMS];
	int   deformOfs = 0, i;

	if (numDeforms > MAX_SHADER_DEFORMS)
	{
		Ren_Warning("GLSL_SetUniform_DeformParms: max MAX_SHADER_DEFORMS reached.");
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
		GLSL_SetUniformFloatARR(trProg.selectedProgram, UNIFORM_DEFORMPARMS, deformParms, MAX_SHADER_DEFORM_PARMS);
	}
}

/**
 * @brief GLSL_SetUniform_ColorModulate
 * @param[in,out] prog
 * @param[in] colorGen
 * @param[in] alphaGen
 */
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

	if ((prog->attributes & ATTR_COLOR) && !(glState.vertexAttribsState & ATTR_COLOR))
	{
		glEnableVertexAttribArray(ATTR_INDEX_COLOR);
		glState.vertexAttribsState |= ATTR_COLOR;
		glVertexAttribPointer(ATTR_INDEX_COLOR, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsColors));
		glState.vertexAttribPointersSet |= ATTR_COLOR;
	}
	else if (!(prog->attributes & ATTR_COLOR) && (glState.vertexAttribsState & ATTR_COLOR))
	{
		glDisableVertexAttribArray(ATTR_INDEX_COLOR);
		glState.vertexAttribsState &= ~ATTR_COLOR;
	}

	SetUniformVec4(UNIFORM_COLORMODULATE, temp);
}

/**
 * @brief GLSL_SetUniform_AlphaTest
 * @param[in] stateBits
 */
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

/**
 * @brief GLSL_VertexAttribsState
 * @param[in] stateBits
 */
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

/**
 * @brief GLSL_VertexAttribPointers
 * @param[in] attribBits
 */
void GLSL_VertexAttribPointers(uint32_t attribBits)
{
	if (!glState.currentVBO)
	{
		// FIXME: this occures on maps for unknown reasons (uje_marketgarden + r_wolffog and
		// and on radar when R_BuildCubemaps is called at start)
		Ren_Warning("GLSL_VertexAttribPointers: no current VBO bound (attribBits %u)\n", attribBits);
		return;
		//Ren_Fatal("GLSL_VertexAttribPointers: no current VBO bound\n");
	}

	Ren_LogComment("--- GLSL_VertexAttribPointers( %s ) ---\n", glState.currentVBO->name);

	if (glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning)
	{
		attribBits |= (ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS);
	}

	if (attribBits & ATTR_POSITION)
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_POSITION )\n");

		glVertexAttribPointer(ATTR_INDEX_POSITION, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsXYZ + (glState.vertexAttribsOldFrame * glState.currentVBO->sizeXYZ)));
		glState.vertexAttribPointersSet |= ATTR_POSITION;
	}

	if (attribBits & ATTR_TEXCOORD)
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_TEXCOORD )\n");

		glVertexAttribPointer(ATTR_INDEX_TEXCOORD0, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsTexCoords));
		glState.vertexAttribPointersSet |= ATTR_TEXCOORD;
	}

	if (attribBits & ATTR_LIGHTCOORD)
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_LIGHTCOORD )\n");

		glVertexAttribPointer(ATTR_INDEX_TEXCOORD1, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsLightCoords));
		glState.vertexAttribPointersSet |= ATTR_LIGHTCOORD;
	}

	if (attribBits & ATTR_TANGENT)
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_TANGENT )\n");

		glVertexAttribPointer(ATTR_INDEX_TANGENT, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsTangents + (glState.vertexAttribsOldFrame * glState.currentVBO->sizeTangents)));
		glState.vertexAttribPointersSet |= ATTR_TANGENT;
	}

	if (attribBits & ATTR_BINORMAL)
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_BINORMAL )\n");

		glVertexAttribPointer(ATTR_INDEX_BINORMAL, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsBinormals + (glState.vertexAttribsOldFrame * glState.currentVBO->sizeBinormals)));
		glState.vertexAttribPointersSet |= ATTR_BINORMAL;
	}

	if (attribBits & ATTR_NORMAL)
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_NORMAL )\n");

		glVertexAttribPointer(ATTR_INDEX_NORMAL, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsNormals + (glState.vertexAttribsOldFrame * glState.currentVBO->sizeNormals)));
		glState.vertexAttribPointersSet |= ATTR_NORMAL;
	}

	if (attribBits & ATTR_COLOR)
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_COLOR )\n");

		glVertexAttribPointer(ATTR_INDEX_COLOR, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsColors));
		glState.vertexAttribPointersSet |= ATTR_COLOR;
	}

	if (attribBits & ATTR_BONE_INDEXES)
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_BONE_INDEXES )\n");

		glVertexAttribPointer(ATTR_INDEX_BONE_INDEXES, 4, GL_INT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsBoneIndexes));
		glState.vertexAttribPointersSet |= ATTR_BONE_INDEXES;
	}

	if (attribBits & ATTR_BONE_WEIGHTS)
	{
		Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_BONE_WEIGHTS )\n");

		glVertexAttribPointer(ATTR_INDEX_BONE_WEIGHTS, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsBoneWeights));
		glState.vertexAttribPointersSet |= ATTR_BONE_WEIGHTS;
	}

	if (glState.vertexAttribsInterpolation > 0)
	{
		if (attribBits & ATTR_POSITION2)
		{
			Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_POSITION2 )\n");

			glVertexAttribPointer(ATTR_INDEX_POSITION2, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsXYZ + (glState.vertexAttribsNewFrame * glState.currentVBO->sizeXYZ)));
			glState.vertexAttribPointersSet |= ATTR_POSITION2;
		}

		if (attribBits & ATTR_TANGENT2)
		{
			Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_TANGENT2 )\n");

			glVertexAttribPointer(ATTR_INDEX_TANGENT2, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsTangents + (glState.vertexAttribsNewFrame * glState.currentVBO->sizeTangents)));
			glState.vertexAttribPointersSet |= ATTR_TANGENT2;
		}

		if (attribBits & ATTR_BINORMAL2)
		{
			Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_BINORMAL2 )\n");

			glVertexAttribPointer(ATTR_INDEX_BINORMAL2, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsBinormals + (glState.vertexAttribsNewFrame * glState.currentVBO->sizeBinormals)));
			glState.vertexAttribPointersSet |= ATTR_BINORMAL2;
		}

		if (attribBits & ATTR_NORMAL2)
		{
			Ren_LogComment("glVertexAttribPointer( ATTR_INDEX_NORMAL2 )\n");

			glVertexAttribPointer(ATTR_INDEX_NORMAL2, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsNormals + (glState.vertexAttribsNewFrame * glState.currentVBO->sizeNormals)));
			glState.vertexAttribPointersSet |= ATTR_NORMAL2;
		}
	}
}
