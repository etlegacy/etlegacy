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
#ifdef RENDERER2C

#include "tr_local.h"

void GLSL_BindNullProgram(void);

enum EGLCompileMacro
{
	USE_ALPHA_TESTING       = BIT(0),
	USE_PORTAL_CLIPPING     = BIT(1),
	USE_FRUSTUM_CLIPPING    = BIT(2),
	USE_VERTEX_SKINNING     = BIT(3),
	USE_VERTEX_ANIMATION    = BIT(4),
	USE_DEFORM_VERTEXES     = BIT(5),
	USE_TCGEN_ENVIRONMENT   = BIT(6),
	USE_TCGEN_LIGHTMAP      = BIT(7),
	USE_NORMAL_MAPPING      = BIT(8),
	USE_PARALLAX_MAPPING    = BIT(9),
	USE_REFLECTIVE_SPECULAR = BIT(10),
	USE_SHADOWING           = BIT(11),
	TWOSIDED                = BIT(12),
	EYE_OUTSIDE             = BIT(13),
	BRIGHTPASS_FILTER       = BIT(14),
	LIGHT_DIRECTIONAL       = BIT(15),
	USE_GBUFFER             = BIT(16),
	MAX_MACROS              = 17
};

typedef struct uniformInfo_s
{
	char *name;
	int type;
}
uniformInfo_t;

// These must be in the same order as in uniform_t in tr_local.h.
static uniformInfo_t uniformsInfo[] =
{
	{ "u_DiffuseMap",                GLSL_INT    },
	{ "u_LightMap",                  GLSL_INT    },
	{ "u_NormalMap",                 GLSL_INT    },
	{ "u_DeluxeMap",                 GLSL_INT    },
	{ "u_SpecularMap",               GLSL_INT    },

	{ "u_TextureMap",                GLSL_INT    },
	{ "u_LevelsMap",                 GLSL_INT    },

	{ "u_ScreenImageMap",            GLSL_INT    },
	{ "u_ScreenDepthMap",            GLSL_INT    },

	{ "u_ShadowMap",                 GLSL_INT    },
	{ "u_ShadowMap2",                GLSL_INT    },
	{ "u_ShadowMap3",                GLSL_INT    },

	{ "u_ShadowMvp",                 GLSL_MAT16  },
	{ "u_ShadowMvp2",                GLSL_MAT16  },
	{ "u_ShadowMvp3",                GLSL_MAT16  },

	{ "u_DiffuseTexMatrix",          GLSL_VEC4   },
	{ "u_DiffuseTexOffTurb",         GLSL_VEC4   },
	{ "u_Texture1Env",               GLSL_INT    },

	{ "u_TCGen0",                    GLSL_INT    },
	{ "u_TCGen0Vector0",             GLSL_VEC3   },
	{ "u_TCGen0Vector1",             GLSL_VEC3   },

	{ "u_DeformGen",                 GLSL_INT    },
	{ "u_DeformParams",              GLSL_FLOAT5 },

	{ "u_ColorGen",                  GLSL_INT    },
	{ "u_AlphaGen",                  GLSL_INT    },
	{ "u_Color",                     GLSL_VEC4   },
	{ "u_BaseColor",                 GLSL_VEC4   },
	{ "u_VertColor",                 GLSL_VEC4   },

	{ "u_DlightInfo",                GLSL_VEC4   },
	{ "u_LightForward",              GLSL_VEC3   },
	{ "u_LightUp",                   GLSL_VEC3   },
	{ "u_LightRight",                GLSL_VEC3   },
	{ "u_LightOrigin",               GLSL_VEC4   },
	{ "u_LightRadius",               GLSL_FLOAT  },
	{ "u_AmbientLight",              GLSL_VEC3   },
	{ "u_DirectedLight",             GLSL_VEC3   },

	{ "u_PortalRange",               GLSL_FLOAT  },

	{ "u_FogDistance",               GLSL_VEC4   },
	{ "u_FogDepth",                  GLSL_VEC4   },
	{ "u_FogEyeT",                   GLSL_FLOAT  },
	{ "u_FogColorMask",              GLSL_VEC4   },

	{ "u_ModelMatrix",               GLSL_MAT16  },
	{ "u_ModelViewProjectionMatrix", GLSL_MAT16  },

	{ "u_Time",                      GLSL_FLOAT  },
	{ "u_VertexLerp",                GLSL_FLOAT  },
	{ "u_MaterialInfo",              GLSL_VEC2   },

	{ "u_ViewInfo",                  GLSL_VEC4   },
	{ "u_ViewOrigin",                GLSL_VEC3   },
	{ "u_ViewForward",               GLSL_VEC3   },
	{ "u_ViewLeft",                  GLSL_VEC3   },
	{ "u_ViewUp",                    GLSL_VEC3   },

	{ "u_InvTexRes",                 GLSL_VEC2   },
	{ "u_AutoExposureMinMax",        GLSL_VEC2   },
	{ "u_ToneMinAvgMaxLinear",       GLSL_VEC3   },

	{ "u_PrimaryLightOrigin",        GLSL_VEC4   },
	{ "u_PrimaryLightColor",         GLSL_VEC3   },
	{ "u_PrimaryLightAmbient",       GLSL_VEC3   },
	{ "u_PrimaryLightRadius",        GLSL_FLOAT  }
};


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
		msg = Ren_Malloc(maxLength);

		qglGetInfoLogARB(object, maxLength, &maxLength, msg);

		for (i = 0; i < maxLength; i += 1024)
		{
			Q_strncpyz(msgPart, msg + i, sizeof(msgPart));

			ri.Printf(printLevel, "%s\n", msgPart);
		}
		ri.Free(msg);
	}
}

static void GLSL_PrintShaderSource(GLhandleARB object)
{
	char        *msg;
	static char msgPart[1024];
	int         maxLength = 0;
	int         i;

	qglGetObjectParameterivARB(object, GL_OBJECT_SHADER_SOURCE_LENGTH_ARB, &maxLength);

	msg = Ren_Malloc(maxLength);

	qglGetShaderSourceARB(object, maxLength, &maxLength, msg);

	for (i = 0; i < maxLength; i += 1024)
	{
		Q_strncpyz(msgPart, msg + i, sizeof(msgPart));
		ri.Printf(PRINT_ALL, "%s\n", msgPart);
	}

	ri.Free(msg);
}

static char *GLSL_GetMacroName(int macro)
{
	switch (macro)
	{
	case USE_ALPHA_TESTING:
		return "USE_ALPHA_TESTING";
	case USE_PORTAL_CLIPPING:
		return "USE_PORTAL_CLIPPING";
	case USE_FRUSTUM_CLIPPING:
		return "USE_FRUSTUM_CLIPPING";
	case USE_VERTEX_SKINNING:
		return "USE_VERTEX_SKINNING";
	case USE_VERTEX_ANIMATION:
		return "USE_VERTEX_ANIMATION";
	case USE_DEFORM_VERTEXES:
		return "USE_DEFORM_VERTEXES";
	case USE_TCGEN_ENVIRONMENT:
		return "USE_TCGEN_ENVIRONMENT";
	case USE_TCGEN_LIGHTMAP:
		return "USE_TCGEN_LIGHTMAP";
	case USE_NORMAL_MAPPING:
		return "USE_NORMAL_MAPPING";
	case USE_PARALLAX_MAPPING:
		return "USE_PARALLAX_MAPPING";
	case USE_REFLECTIVE_SPECULAR:
		return "USE_REFLECTIVE_SPECULAR";
	case USE_SHADOWING:
		return "USE_SHADOWING";
	case TWOSIDED:
		return "TWOSIDED";
	case EYE_OUTSIDE:
		return "EYE_OUTSIDE";
	case BRIGHTPASS_FILTER:
		return "BRIGHTPASS_FILTER";
	case LIGHT_DIRECTIONAL:
		return "LIGHT_DIRECTIONAL";
	case USE_GBUFFER:
		return "USE_GBUFFER";
	default:
		return NULL;
	}
}

static qboolean GLSL_HasConflictingMacros(int compilemacro, int permutation)
{
	switch (compilemacro)
	{
	case USE_VERTEX_SKINNING:
		if (permutation & compilemacro && compilemacro == USE_VERTEX_ANIMATION)
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
		if (permutation & compilemacro && compilemacro == USE_VERTEX_SKINNING)
		{
			return qtrue;
		}
		break;
	default:
		return qfalse;
	}

	return qfalse;
}

static qboolean GLSL_MissesRequiredMacros(int compilemacro, int permutation)
{
	switch (compilemacro)
	{
	case USE_PARALLAX_MAPPING:
		if (permutation & compilemacro && compilemacro == USE_NORMAL_MAPPING)
		{
			return qtrue;
		}
		break;
	case USE_REFLECTIVE_SPECULAR:
		if (permutation & compilemacro && compilemacro == USE_NORMAL_MAPPING)
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
	case TWOSIDED:
		attr = ATTR_NORMAL;
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

	char *bufferFinal = NULL;
	int  sizeFinal;

	float fbufWidthScale, fbufHeightScale;
	float npotWidthScale, npotHeightScale;

	Com_Memset(bufferExtra, 0, sizeof(bufferExtra));

#if defined(COMPAT_Q3A) || defined(COMPAT_ET)
	Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef COMPAT_Q3A\n#define COMPAT_Q3A 1\n#endif\n");
#endif

	Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef COMPAT_ET\n#define COMPAT_ET 1\n#endif\n");

	// HACK: add some macros to avoid extra uniforms and save speed and code maintenance
	Q_strcat(bufferExtra, sizeof(bufferExtra),
	         va("#ifndef r_SpecularExponent\n#define r_SpecularExponent %f\n#endif\n", r_specularExponent->value));

	Q_strcat(bufferExtra, sizeof(bufferExtra),
	         va("#ifndef r_SpecularExponent2\n#define r_SpecularExponent2 %f\n#endif\n", r_specularExponent2->value));

	Q_strcat(bufferExtra, sizeof(bufferExtra),
	         va("#ifndef r_SpecularScale\n#define r_SpecularScale %f\n#endif\n", r_specularScale->value));
	//Q_strcat(bufferExtra, sizeof(bufferExtra),
	//       va("#ifndef r_NormalScale\n#define r_NormalScale %f\n#endif\n", r_normalScale->value));

	Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef M_PI\n#define M_PI 3.14159265358979323846f\n#endif\n");

	Q_strcat(bufferExtra, sizeof(bufferExtra), va("#ifndef MAX_SHADOWMAPS\n#define MAX_SHADOWMAPS %i\n#endif\n", MAX_SHADOWMAPS));

	Q_strcat(bufferExtra, sizeof(bufferExtra), va("#ifndef MAX_SHADER_DEFORM_PARMS\n#define MAX_SHADER_DEFORM_PARMS %i\n#endif\n", MAX_SHADER_DEFORM_PARMS));

	Q_strcat(bufferExtra, sizeof(bufferExtra),
	         va("#ifndef deform_t\n"
	            "#define deform_t\n"
	            "#define DEFORM_WAVE %i\n"
	            "#define DEFORM_BULGE %i\n"
	            "#define DEFORM_MOVE %i\n"
	            "#endif\n",
	            DEFORM_WAVE,
	            DEFORM_BULGE,
	            DEFORM_MOVE));

	Q_strcat(bufferExtra, sizeof(bufferExtra),
	         va("#ifndef genFunc_t\n"
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
	            ( float ) GF_NOISE));

	/*
	Q_strcat(bufferExtra, sizeof(bufferExtra),
	                                 va("#ifndef deformGen_t\n"
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
	                                        DGEN_MOVE));
	                                */

	/*
	Q_strcat(bufferExtra, sizeof(bufferExtra),
	                                 va("#ifndef colorGen_t\n"
	                                        "#define colorGen_t\n"
	                                        "#define CGEN_VERTEX %i\n"
	                                        "#define CGEN_ONE_MINUS_VERTEX %i\n"
	                                        "#endif\n",
	                                        CGEN_VERTEX,
	                                        CGEN_ONE_MINUS_VERTEX));

	Q_strcat(bufferExtra, sizeof(bufferExtra),
	                                                 va("#ifndef alphaGen_t\n"
	                                                        "#define alphaGen_t\n"
	                                                        "#define AGEN_VERTEX %i\n"
	                                                        "#define AGEN_ONE_MINUS_VERTEX %i\n"
	                                                        "#endif\n",
	                                                        AGEN_VERTEX,
	                                                        AGEN_ONE_MINUS_VERTEX));
	                                                        */

	Q_strcat(bufferExtra, sizeof(bufferExtra),
	         va("#ifndef alphaTest_t\n"
	            "#define alphaTest_t\n"
	            "#define ATEST_GT_0 %i\n"
	            "#define ATEST_LT_128 %i\n"
	            "#define ATEST_GE_128 %i\n"
	            "#endif\n",
	            ATEST_GT_0,
	            ATEST_LT_128,
	            ATEST_GE_128));

	fbufWidthScale  = Q_recip(( float ) glConfig.vidWidth);
	fbufHeightScale = Q_recip(( float ) glConfig.vidHeight);
	Q_strcat(bufferExtra, sizeof(bufferExtra),
	         va("#ifndef r_FBufScale\n#define r_FBufScale vec2(%f, %f)\n#endif\n", fbufWidthScale, fbufHeightScale));

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

	Q_strcat(bufferExtra, sizeof(bufferExtra),
	         va("#ifndef r_NPOTScale\n#define r_NPOTScale vec2(%f, %f)\n#endif\n", npotWidthScale, npotHeightScale));

	if (glConfig.driverType == GLDRV_MESA)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef GLDRV_MESA\n#define GLDRV_MESA 1\n#endif\n");
	}

	if (glConfig.hardwareType == GLHW_ATI)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef GLHW_ATI\n#define GLHW_ATI 1\n#endif\n");
	}
	else if (glConfig.hardwareType == GLHW_ATI_DX10)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef GLHW_ATI_DX10\n#define GLHW_ATI_DX10 1\n#endif\n");
	}
	else if (glConfig.hardwareType == GLHW_NV_DX10)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef GLHW_NV_DX10\n#define GLHW_NV_DX10 1\n#endif\n");
	}

	if (r_shadows->integer >= SHADOWING_ESM16 && glConfig2.textureFloatAvailable && glConfig2.framebufferObjectAvailable)
	{
		if (r_shadows->integer == SHADOWING_ESM16 || r_shadows->integer == SHADOWING_ESM32)
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef ESM\n#define ESM 1\n#endif\n");
		}
		else if (r_shadows->integer == SHADOWING_EVSM32)
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef EVSM\n#define EVSM 1\n#endif\n");

			// The exponents for the EVSM techniques should be less than ln(FLT_MAX/FILTER_SIZE)/2 {ln(FLT_MAX/1)/2 ~44.3}
			//         42.9 is the maximum possible value for FILTER_SIZE=15
			//         42.0 is the truncated value that we pass into the sample
			Q_strcat(bufferExtra, sizeof(bufferExtra),
			         va("#ifndef r_EVSMExponents\n#define r_EVSMExponents vec2(%f, %f)\n#endif\n", 42.0f, 42.0f));

			if (r_evsmPostProcess->integer)
			{
				Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_EVSMPostProcess\n#define r_EVSMPostProcess 1\n#endif\n");
			}
		}
		else
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef VSM\n#define VSM 1\n#endif\n");

			if (glConfig.hardwareType == GLHW_ATI)
			{
				Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef VSM_CLAMP\n#define VSM_CLAMP 1\n#endif\n");
			}
		}

		if ((glConfig.hardwareType == GLHW_NV_DX10 || glConfig.hardwareType == GLHW_ATI_DX10) && r_shadows->integer == SHADOWING_VSM32)
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef VSM_EPSILON\n#define VSM_EPSILON 0.000001\n#endif\n");
		}
		else
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef VSM_EPSILON\n#define VSM_EPSILON 0.0001\n#endif\n");
		}

		if (r_lightBleedReduction->value)
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra),
			         va("#ifndef r_LightBleedReduction\n#define r_LightBleedReduction %f\n#endif\n",
			            r_lightBleedReduction->value));
		}

		if (r_overDarkeningFactor->value)
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra),
			         va("#ifndef r_OverDarkeningFactor\n#define r_OverDarkeningFactor %f\n#endif\n",
			            r_overDarkeningFactor->value));
		}

		if (r_shadowMapDepthScale->value)
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra),
			         va("#ifndef r_ShadowMapDepthScale\n#define r_ShadowMapDepthScale %f\n#endif\n",
			            r_shadowMapDepthScale->value));
		}

		if (r_debugShadowMaps->integer)
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra),
			         va("#ifndef r_DebugShadowMaps\n#define r_DebugShadowMaps %i\n#endif\n", r_debugShadowMaps->integer));
		}

		/*
		if(r_softShadows->integer == 1)
		{
		        Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef PCF_2X2\n#define PCF_2X2 1\n#endif\n");
		}
		else if(r_softShadows->integer == 2)
		{
		        Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef PCF_3X3\n#define PCF_3X3 1\n#endif\n");
		}
		else if(r_softShadows->integer == 3)
		{
		        Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef PCF_4X4\n#define PCF_4X4 1\n#endif\n");
		}
		else if(r_softShadows->integer == 4)
		{
		        Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef PCF_5X5\n#define PCF_5X5 1\n#endif\n");
		}
		else if(r_softShadows->integer == 5)
		{
		        Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef PCF_6X6\n#define PCF_6X6 1\n#endif\n");
		}
		*/
		if (r_softShadows->integer == 6)
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef PCSS\n#define PCSS 1\n#endif\n");
		}
		else if (r_softShadows->integer)
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra),
			         va("#ifndef r_PCFSamples\n#define r_PCFSamples %1.1f\n#endif\n", r_softShadows->value + 1.0f));
		}

		if (r_parallelShadowSplits->integer)
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra),
			         va("#ifndef r_ParallelShadowSplits_%i\n#define r_ParallelShadowSplits_%i\n#endif\n", r_parallelShadowSplits->integer, r_parallelShadowSplits->integer));
		}

		if (r_showParallelShadowSplits->integer)
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_ShowParallelShadowSplits\n#define r_ShowParallelShadowSplits 1\n#endif\n");
		}
	}

	if (r_deferredShading->integer && glConfig2.maxColorAttachments >= 4 && glConfig2.textureFloatAvailable &&
	    glConfig2.drawBuffersAvailable && glConfig2.maxDrawBuffers >= 4)
	{
		if (r_deferredShading->integer == DS_STANDARD)
		{
			Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_DeferredShading\n#define r_DeferredShading 1\n#endif\n");
		}
	}

	if (r_hdrRendering->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_HDRRendering\n#define r_HDRRendering 1\n#endif\n");

		Q_strcat(bufferExtra, sizeof(bufferExtra),
		         va("#ifndef r_HDRContrastThreshold\n#define r_HDRContrastThreshold %f\n#endif\n",
		            r_hdrContrastThreshold->value));

		Q_strcat(bufferExtra, sizeof(bufferExtra),
		         va("#ifndef r_HDRContrastOffset\n#define r_HDRContrastOffset %f\n#endif\n",
		            r_hdrContrastOffset->value));

		Q_strcat(bufferExtra, sizeof(bufferExtra),
		         va("#ifndef r_HDRToneMappingOperator\n#define r_HDRToneMappingOperator_%i\n#endif\n",
		            r_hdrToneMappingOperator->integer));

		Q_strcat(bufferExtra, sizeof(bufferExtra),
		         va("#ifndef r_HDRGamma\n#define r_HDRGamma %f\n#endif\n",
		            r_hdrGamma->value));
	}

	if (r_precomputedLighting->integer)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra),
		         "#ifndef r_precomputedLighting\n#define r_precomputedLighting 1\n#endif\n");
	}

	if (r_heatHazeFix->integer && glConfig2.framebufferBlitAvailable && /*glConfig.hardwareType != GLHW_ATI && glConfig.hardwareType != GLHW_ATI_DX10 &&*/ glConfig.driverType != GLDRV_MESA)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_heatHazeFix\n#define r_heatHazeFix 1\n#endif\n");
	}

	if (r_showLightMaps->integer)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_showLightMaps\n#define r_showLightMaps 1\n#endif\n");
	}

	if (r_showDeluxeMaps->integer)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_showDeluxeMaps\n#define r_showDeluxeMaps 1\n#endif\n");
	}

#ifdef EXPERIMENTAL

	if (r_screenSpaceAmbientOcclusion->integer)
	{
		int             i;
		static vec3_t   jitter[32];
		static qboolean jitterInit = qfalse;

		if (!jitterInit)
		{
			for (i = 0; i < 32; i++)
			{
				float *jit = &jitter[i][0];

				float rad = crandom() * 1024.0f;     // FIXME radius;
				float a   = crandom() * M_PI * 2;
				float b   = crandom() * M_PI * 2;

				jit[0] = rad * sin(a) * cos(b);
				jit[1] = rad * sin(a) * sin(b);
				jit[2] = rad * cos(a);
			}

			jitterInit = qtrue;
		}

		// TODO
	}

#endif

	if (glConfig2.vboVertexSkinningAvailable)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_VertexSkinning\n#define r_VertexSkinning 1\n#endif\n");

		Q_strcat(bufferExtra, sizeof(bufferExtra),
		         va("#ifndef MAX_GLSL_BONES\n#define MAX_GLSL_BONES %i\n#endif\n", glConfig2.maxVertexSkinningBones));
	}
	else
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra),
		         va("#ifndef MAX_GLSL_BONES\n#define MAX_GLSL_BONES %i\n#endif\n", 4));
	}

	/*
	   if(glConfig.drawBuffersAvailable && glConfig.maxDrawBuffers >= 4)
	   {
	   //Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef GL_ARB_draw_buffers\n#define GL_ARB_draw_buffers 1\n#endif\n");
	   Q_strcat(bufferExtra, sizeof(bufferExtra), "#extension GL_ARB_draw_buffers : enable\n");
	   }
	 */

	if (r_normalMapping->integer)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_NormalMapping\n#define r_NormalMapping 1\n#endif\n");
	}

	if (/* TODO: check for shader model 3 hardware  && */ r_normalMapping->integer && r_parallaxMapping->integer)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_ParallaxMapping\n#define r_ParallaxMapping 1\n#endif\n");
	}

	if (r_wrapAroundLighting->value)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra),
		         va("#ifndef r_WrapAroundLighting\n#define r_WrapAroundLighting %f\n#endif\n",
		            r_wrapAroundLighting->value));
	}

	if (r_halfLambertLighting->integer)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_HalfLambertLighting\n#define r_HalfLambertLighting 1\n#endif\n");
	}

	if (r_rimLighting->integer)
	{
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_RimLighting\n#define r_RimLighting 1\n#endif\n");
		Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef r_RimColor\n#define r_RimColor vec4(0.26, 0.19, 0.16, 0.0)\n#endif\n");
		Q_strcat(bufferExtra, sizeof(bufferExtra), va("#ifndef r_RimExponent\n#define r_RimExponent %f\n#endif\n",
		                                              r_rimExponent->value));
	}

	// OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
	// so we have to reset the line counting
	Q_strcat(bufferExtra, sizeof(bufferExtra), "#line 0\n");

	*size    = strlen(bufferExtra) + 1;
	*defines = (char *) malloc(*size);
	memset(*defines, 0, *size);
	Q_strcat(*defines, *size, bufferExtra);
}

static void GLSL_GetShaderHeader(GLenum shaderType, char *dest, int size)
{
	char *bufferExtra;
	int  sizeExtra;

	dest[0] = '\0';

	// HACK: abuse the GLSL preprocessor to turn GLSL 1.20 shaders into 1.30 ones
	if (glRefConfig.glslMajorVersion > 1 || (glRefConfig.glslMajorVersion == 1 && glRefConfig.glslMinorVersion >= 30))
	{
		Q_strcat(dest, size, "#version 130\n");

		if (shaderType == GL_VERTEX_SHADER_ARB)
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

static int GLSL_CompileGPUShader(GLhandleARB program, GLhandleARB *prevShader, const GLcharARB *buffer, int size, GLenum shaderType)
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
		GLSL_PrintShaderSource(shader);
		GLSL_PrintInfoLog(shader, qfalse);
		ri.Error(ERR_DROP, "Couldn't compile shader");
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
	int  dataSize;
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
				memset(*data + *size, 0, strl);

			}
			else
			{
				*data = (char *) malloc(strl);
				memset(*data, 0, strl);
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
			memset(*data + *size, 0, dataSize);
		}
		else
		{
			*data = (char *) malloc(dataSize);
			memset(*data, 0, dataSize);
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
	{
		char *shaderExtra = NULL;
		int  extraSize    = 0;

		char *bufferFinal = NULL;
		int  sizeFinal;

		GLSL_GetShaderExtraDefines(&shaderExtra, &extraSize);

		sizeFinal = extraSize + mainSize + libsSize;

		bufferFinal = ( char * ) ri.Hunk_AllocateTempMemory(sizeFinal);

		strcpy(bufferFinal, shaderExtra);

		if (libsSize > 0)
		{
			Q_strcat(bufferFinal, sizeFinal, libsBuffer);
		}

		Q_strcat(bufferFinal, sizeFinal, mainBuffer);

		shaderText = malloc(sizeFinal);
		strcpy(shaderText, bufferFinal);
		ri.Hunk_FreeTempMemory(bufferFinal);
		free(shaderExtra);
	}
	free(mainBuffer);
	free(libsBuffer);

	return shaderText;
}

static qboolean GLSL_GenerateMacroString(const char *macros, int marcoatrib, int permutation)
{
	return qtrue;
}

static void GLSL_LinkProgram(GLhandleARB program)
{
	GLint linked;

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

		ri.Printf(PRINT_DEVELOPER, "active uniform: '%s'\n", uniformName);
	}

	qglUseProgramObjectARB(0);
}

static int GLSL_InitGPUShader2(shaderProgram_t *program, const char *name, int attribs, const char *vpCode, const char *fpCode)
{
	ri.Printf(PRINT_DEVELOPER, "------- GPU shader -------\n");

	if (strlen(name) >= MAX_QPATH)
	{
		ri.Error(ERR_DROP, "GLSL_InitGPUShader2: \"%s\" is too long", name);
	}

	Q_strncpyz(program->name, name, sizeof(program->name));

	program->program = qglCreateProgramObjectARB();
	program->attribs = attribs;

	if (!(GLSL_CompileGPUShader(program->program, &program->vertexShader, vpCode, strlen(vpCode), GL_VERTEX_SHADER_ARB)))
	{
		ri.Printf(PRINT_ALL, "GLSL_InitGPUShader2: Unable to load \"%s\" as GL_VERTEX_SHADER_ARB\n", name);
		qglDeleteObjectARB(program->program);
		return 0;
	}

	if (fpCode)
	{
		if (!(GLSL_CompileGPUShader(program->program, &program->fragmentShader, fpCode, strlen(fpCode), GL_FRAGMENT_SHADER_ARB)))
		{
			ri.Printf(PRINT_ALL, "GLSL_InitGPUShader2: Unable to load \"%s\" as GL_FRAGMENT_SHADER_ARB\n", name);
			qglDeleteObjectARB(program->program);
			return 0;
		}
	}

	if (attribs & ATTR_POSITION)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_POSITION, "attr_Position");
	}

	if (attribs & ATTR_TEXCOORD)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TEXCOORD0, "attr_TexCoord0");
	}

	if (attribs & ATTR_LIGHTCOORD)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TEXCOORD1, "attr_TexCoord1");
	}

//  if(attribs & ATTR_TEXCOORD2)
//      qglBindAttribLocationARB(program->program, ATTR_INDEX_TEXCOORD2, "attr_TexCoord2");

//  if(attribs & ATTR_TEXCOORD3)
//      qglBindAttribLocationARB(program->program, ATTR_INDEX_TEXCOORD3, "attr_TexCoord3");

#ifdef USE_VERT_TANGENT_SPACE
	if (attribs & ATTR_TANGENT)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TANGENT, "attr_Tangent");
	}

	if (attribs & ATTR_BITANGENT)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_BITANGENT, "attr_Bitangent");
	}
#endif

	if (attribs & ATTR_NORMAL)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_NORMAL, "attr_Normal");
	}

	if (attribs & ATTR_COLOR)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_COLOR, "attr_Color");
	}

	if (attribs & ATTR_PAINTCOLOR)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_PAINTCOLOR, "attr_PaintColor");
	}

	if (attribs & ATTR_LIGHTDIRECTION)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_LIGHTDIRECTION, "attr_LightDirection");
	}

	if (attribs & ATTR_POSITION2)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_POSITION2, "attr_Position2");
	}

	if (attribs & ATTR_NORMAL2)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_NORMAL2, "attr_Normal2");
	}

#ifdef USE_VERT_TANGENT_SPACE
	if (attribs & ATTR_TANGENT2)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TANGENT2, "attr_Tangent2");
	}

	if (attribs & ATTR_BITANGENT2)
	{
		qglBindAttribLocationARB(program->program, ATTR_INDEX_BITANGENT2, "attr_Bitangent2");
	}
#endif

	GLSL_LinkProgram(program->program);

	return 1;
}
/*
static int GLSL_InitGPUShader(shaderProgram_t *program, const char *name,
                              int attribs, qboolean fragmentShader, const GLcharARB *extra, qboolean addHeader,
                              const char *fallback_vp, const char *fallback_fp)
{
    char vpCode[32000];
    char fpCode[32000];
    char *postHeader;
    int  size;
    int  result;

    size = sizeof(vpCode);
    if (addHeader)
    {
        GLSL_GetShaderHeader(GL_VERTEX_SHADER_ARB, extra, vpCode, size);
        postHeader = &vpCode[strlen(vpCode)];
        size      -= strlen(vpCode);
    }
    else
    {
        postHeader = &vpCode[0];
    }

    if (!GLSL_LoadGPUShaderText(name, fallback_vp, GL_VERTEX_SHADER_ARB, postHeader, size))
    {
        return 0;
    }

    if (fragmentShader)
    {
        size = sizeof(fpCode);
        if (addHeader)
        {
            GLSL_GetShaderHeader(GL_FRAGMENT_SHADER_ARB, extra, fpCode, size);
            postHeader = &fpCode[strlen(fpCode)];
            size      -= strlen(fpCode);
        }
        else
        {
            postHeader = &fpCode[0];
        }

        if (!GLSL_LoadGPUShaderText(name, fallback_fp, GL_FRAGMENT_SHADER_ARB, postHeader, size))
        {
            return 0;
        }
    }

    result = GLSL_InitGPUShader2(program, name, attribs, vpCode, fragmentShader ? fpCode : NULL);

    return result;
}
*/

static qboolean GLSL_InitGPUShader(shaderProgramList_t program, const char *name, int attribs, const char *libs, int macros, const char *macrostring)
{
	char   *vertexShader   = GLSL_BuildGPUShaderText(name, libs, GL_VERTEX_SHADER);
	char   *fragmentShader = GLSL_BuildGPUShaderText(name, libs, GL_FRAGMENT_SHADER);
	int    macronum        = 0;
	int    startTime, endTime;
	size_t numPermutations = 0, numCompiled = 0, tics = 0, nextTicCount = 0;
	int    i               = 0;
	if (macros)
	{
		for (i = 0; i < MAX_MACROS; i++)
		{
			if (macros & BIT(i))
			{
				macronum++;
			}
		}
	}

	startTime = ri.Milliseconds();

	numPermutations = BIT(macronum);

	ri.Printf(PRINT_ALL, "...compiling %s shaders\n", name);
	ri.Printf(PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n");
	ri.Printf(PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n");

	for (i = 0; i < numPermutations; i++)
	{
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

		if (GLSL_GenerateMacroString(macrostring, macros, i))
		{

			numCompiled++;
		}

	}
	endTime = ri.Milliseconds();
	ri.Printf(PRINT_ALL, "...compiled %i %s shader permutations in %5.2f seconds\n", ( int ) numCompiled, name, (endTime - startTime) / 1000.0);
	return qtrue;
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

	program->uniformBuffer = Ren_Malloc(size);
}

void GLSL_FinishGPUShader(shaderProgram_t *program)
{
	GLSL_ValidateProgram(program->program);
	GLSL_ShowProgramUniforms(program->program);
	GL_CheckErrors();
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
		ri.Printf(PRINT_WARNING, "GLSL_SetUniformInt: wrong type for uniform %i in program %s\n", uniformNum, program->name);
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
		ri.Printf(PRINT_WARNING, "GLSL_SetUniformFloat: wrong type for uniform %i in program %s\n", uniformNum, program->name);
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
		ri.Printf(PRINT_WARNING, "GLSL_SetUniformVec2: wrong type for uniform %i in program %s\n", uniformNum, program->name);
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
		ri.Printf(PRINT_WARNING, "GLSL_SetUniformVec3: wrong type for uniform %i in program %s\n", uniformNum, program->name);
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
		ri.Printf(PRINT_WARNING, "GLSL_SetUniformVec4: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (VectorCompare4(v, compare))
	{
		return;
	}

	VectorCopy4(v, compare);

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
		ri.Printf(PRINT_WARNING, "GLSL_SetUniformFloat5: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (VectorCompare5(v, compare))
	{
		return;
	}

	VectorCopy5(v, compare);

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
		ri.Printf(PRINT_WARNING, "GLSL_SetUniformMatrix16: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (Matrix16Compare(matrix, compare))
	{
		return;
	}

	Matrix16Copy(matrix, compare);

	qglUniformMatrix4fvARB(uniforms[uniformNum], 1, GL_FALSE, matrix);
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
			ri.Free(program->uniformBuffer);
		}

		Com_Memset(program, 0, sizeof(*program));
	}
}

void GLSL_InitGPUShaders(void)
{
	int  startTime, endTime;
	int  i;
	char extradefines[1024];
	int  attribs;
	int  numGenShaders = 0, numLightShaders = 0, numEtcShaders = 0;

	ri.Printf(PRINT_ALL, "------- GLSL_InitGPUShaders -------\n");

	R_IssuePendingRenderCommands();

	startTime = ri.Milliseconds();

	endTime = ri.Milliseconds();

	ri.Printf(PRINT_ALL, "loaded %i GLSL shaders (%i gen %i light %i etc) in %5.2f seconds\n",
	          numGenShaders + numLightShaders + numEtcShaders, numGenShaders, numLightShaders,
	          numEtcShaders, (endTime - startTime) / 1000.0);
}

void GLSL_ShutdownGPUShaders(void)
{
	int i;

	ri.Printf(PRINT_ALL, "------- GLSL_ShutdownGPUShaders -------\n");

	qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD0);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD1);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_POSITION);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_POSITION2);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_NORMAL);
#ifdef USE_VERT_TANGENT_SPACE
	qglDisableVertexAttribArrayARB(ATTR_INDEX_TANGENT);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_BITANGENT);
#endif
	qglDisableVertexAttribArrayARB(ATTR_INDEX_NORMAL2);
#ifdef USE_VERT_TANGENT_SPACE
	qglDisableVertexAttribArrayARB(ATTR_INDEX_TANGENT2);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_BITANGENT2);
#endif
	qglDisableVertexAttribArrayARB(ATTR_INDEX_COLOR);
	qglDisableVertexAttribArrayARB(ATTR_INDEX_LIGHTDIRECTION);
	GLSL_BindNullProgram();

	for (i = 0; i < GENERICDEF_COUNT; i++)
		GLSL_DeleteGPUShader(&tr.genericShader[i]);

	GLSL_DeleteGPUShader(&tr.textureColorShader);

	for (i = 0; i < FOGDEF_COUNT; i++)
		GLSL_DeleteGPUShader(&tr.fogShader[i]);

	for (i = 0; i < DLIGHTDEF_COUNT; i++)
		GLSL_DeleteGPUShader(&tr.dlightShader[i]);

	for (i = 0; i < LIGHTDEF_COUNT; i++)
		GLSL_DeleteGPUShader(&tr.lightallShader[i]);

	GLSL_DeleteGPUShader(&tr.shadowmapShader);
	GLSL_DeleteGPUShader(&tr.pshadowShader);
	GLSL_DeleteGPUShader(&tr.down4xShader);
	GLSL_DeleteGPUShader(&tr.bokehShader);
	GLSL_DeleteGPUShader(&tr.tonemapShader);

	for (i = 0; i < 2; i++)
		GLSL_DeleteGPUShader(&tr.calclevels4xShader[i]);

	GLSL_DeleteGPUShader(&tr.shadowmaskShader);
	GLSL_DeleteGPUShader(&tr.ssaoShader);

	for (i = 0; i < 2; i++)
		GLSL_DeleteGPUShader(&tr.depthBlurShader[i]);

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

	if (r_logFile->integer)
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment(va("--- GL_BindProgram( %s ) ---\n", program->name));
	}

	if (glState.currentProgram != program)
	{
		qglUseProgramObjectARB(program->program);
		glState.currentProgram = program;
		backEnd.pc.c_glslShaderBinds++;
	}
}


void GLSL_BindNullProgram(void)
{
	if (r_logFile->integer)
	{
		GLimp_LogComment("--- GL_BindNullProgram ---\n");
	}

	if (glState.currentProgram)
	{
		qglUseProgramObjectARB(0);
		glState.currentProgram = NULL;
	}
}


void GLSL_VertexAttribsState(uint32_t stateBits)
{
	uint32_t diff;

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
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_POSITION )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_POSITION);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_POSITION )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_POSITION);
		}
	}

	if (diff & ATTR_TEXCOORD)
	{
		if (stateBits & ATTR_TEXCOORD)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_TEXCOORD )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD0);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_TEXCOORD )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD0);
		}
	}

	if (diff & ATTR_LIGHTCOORD)
	{
		if (stateBits & ATTR_LIGHTCOORD)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_LIGHTCOORD )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD1);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_LIGHTCOORD )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD1);
		}
	}

	if (diff & ATTR_NORMAL)
	{
		if (stateBits & ATTR_NORMAL)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_NORMAL )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_NORMAL);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_NORMAL )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_NORMAL);
		}
	}

#ifdef USE_VERT_TANGENT_SPACE
	if (diff & ATTR_TANGENT)
	{
		if (stateBits & ATTR_TANGENT)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_TANGENT )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_TANGENT);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_TANGENT )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_TANGENT);
		}
	}

	if (diff & ATTR_BITANGENT)
	{
		if (stateBits & ATTR_BITANGENT)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_BITANGENT )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_BITANGENT);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_BITANGENT )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_BITANGENT);
		}
	}
#endif

	if (diff & ATTR_COLOR)
	{
		if (stateBits & ATTR_COLOR)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_COLOR )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_COLOR);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_COLOR )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_COLOR);
		}
	}

	if (diff & ATTR_LIGHTDIRECTION)
	{
		if (stateBits & ATTR_LIGHTDIRECTION)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_LIGHTDIRECTION )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_LIGHTDIRECTION);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_LIGHTDIRECTION )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_LIGHTDIRECTION);
		}
	}

	if (diff & ATTR_POSITION2)
	{
		if (stateBits & ATTR_POSITION2)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_POSITION2 )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_POSITION2);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_POSITION2 )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_POSITION2);
		}
	}

	if (diff & ATTR_NORMAL2)
	{
		if (stateBits & ATTR_NORMAL2)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_NORMAL2 )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_NORMAL2);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_NORMAL2 )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_NORMAL2);
		}
	}

#ifdef USE_VERT_TANGENT_SPACE
	if (diff & ATTR_TANGENT2)
	{
		if (stateBits & ATTR_TANGENT2)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_TANGENT2 )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_TANGENT2);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_TANGENT2 )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_TANGENT2);
		}
	}

	if (diff & ATTR_BITANGENT2)
	{
		if (stateBits & ATTR_BITANGENT2)
		{
			GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_BITANGENT2 )\n");
			qglEnableVertexAttribArrayARB(ATTR_INDEX_BITANGENT2);
		}
		else
		{
			GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_BITANGENT2 )\n");
			qglDisableVertexAttribArrayARB(ATTR_INDEX_BITANGENT2);
		}
	}
#endif

	glState.vertexAttribsState = stateBits;
}

void GLSL_VertexAttribPointers(uint32_t attribBits)
{
	qboolean animated;
	int      newFrame, oldFrame;

	if (!glState.currentVBO)
	{
		ri.Error(ERR_FATAL, "GL_VertexAttribPointers: no VBO bound");
		return;
	}

	// don't just call LogComment, or we will get a call to va() every frame!
	GLimp_LogComment(va("--- GL_VertexAttribPointers( %s ) ---\n", glState.currentVBO->name));

	// position/normal/tangent/bitangent are always set in case of animation
	oldFrame = glState.vertexAttribsOldFrame;
	newFrame = glState.vertexAttribsNewFrame;
	animated = (oldFrame != newFrame) && (glState.vertexAttribsInterpolation > 0.0f);

	if ((attribBits & ATTR_POSITION) && (!(glState.vertexAttribPointersSet & ATTR_POSITION) || animated))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_POSITION )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_POSITION, 3, GL_FLOAT, 0, glState.currentVBO->stride_xyz, BUFFER_OFFSET(glState.currentVBO->ofs_xyz + newFrame * glState.currentVBO->size_xyz));
		glState.vertexAttribPointersSet |= ATTR_POSITION;
	}

	if ((attribBits & ATTR_TEXCOORD) && !(glState.vertexAttribPointersSet & ATTR_TEXCOORD))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_TEXCOORD )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_TEXCOORD0, 2, GL_FLOAT, 0, glState.currentVBO->stride_st, BUFFER_OFFSET(glState.currentVBO->ofs_st));
		glState.vertexAttribPointersSet |= ATTR_TEXCOORD;
	}

	if ((attribBits & ATTR_LIGHTCOORD) && !(glState.vertexAttribPointersSet & ATTR_LIGHTCOORD))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_LIGHTCOORD )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_TEXCOORD1, 2, GL_FLOAT, 0, glState.currentVBO->stride_lightmap, BUFFER_OFFSET(glState.currentVBO->ofs_lightmap));
		glState.vertexAttribPointersSet |= ATTR_LIGHTCOORD;
	}

	if ((attribBits & ATTR_NORMAL) && (!(glState.vertexAttribPointersSet & ATTR_NORMAL) || animated))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_NORMAL )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_NORMAL, 3, GL_FLOAT, 0, glState.currentVBO->stride_normal, BUFFER_OFFSET(glState.currentVBO->ofs_normal + newFrame * glState.currentVBO->size_normal));
		glState.vertexAttribPointersSet |= ATTR_NORMAL;
	}

#ifdef USE_VERT_TANGENT_SPACE
	if ((attribBits & ATTR_TANGENT) && (!(glState.vertexAttribPointersSet & ATTR_TANGENT) || animated))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_TANGENT )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_TANGENT, 3, GL_FLOAT, 0, glState.currentVBO->stride_tangent, BUFFER_OFFSET(glState.currentVBO->ofs_tangent + newFrame * glState.currentVBO->size_normal)); // FIXME
		glState.vertexAttribPointersSet |= ATTR_TANGENT;
	}

	if ((attribBits & ATTR_BITANGENT) && (!(glState.vertexAttribPointersSet & ATTR_BITANGENT) || animated))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_BITANGENT )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_BITANGENT, 3, GL_FLOAT, 0, glState.currentVBO->stride_bitangent, BUFFER_OFFSET(glState.currentVBO->ofs_bitangent + newFrame * glState.currentVBO->size_normal)); // FIXME
		glState.vertexAttribPointersSet |= ATTR_BITANGENT;
	}
#endif

	if ((attribBits & ATTR_COLOR) && !(glState.vertexAttribPointersSet & ATTR_COLOR))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_COLOR )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_COLOR, 4, GL_FLOAT, 0, glState.currentVBO->stride_vertexcolor, BUFFER_OFFSET(glState.currentVBO->ofs_vertexcolor));
		glState.vertexAttribPointersSet |= ATTR_COLOR;
	}

	if ((attribBits & ATTR_LIGHTDIRECTION) && !(glState.vertexAttribPointersSet & ATTR_LIGHTDIRECTION))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_LIGHTDIRECTION )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_LIGHTDIRECTION, 3, GL_FLOAT, 0, glState.currentVBO->stride_lightdir, BUFFER_OFFSET(glState.currentVBO->ofs_lightdir));
		glState.vertexAttribPointersSet |= ATTR_LIGHTDIRECTION;
	}

	if ((attribBits & ATTR_POSITION2) && (!(glState.vertexAttribPointersSet & ATTR_POSITION2) || animated))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_POSITION2 )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_POSITION2, 3, GL_FLOAT, 0, glState.currentVBO->stride_xyz, BUFFER_OFFSET(glState.currentVBO->ofs_xyz + oldFrame * glState.currentVBO->size_xyz));
		glState.vertexAttribPointersSet |= ATTR_POSITION2;
	}

	if ((attribBits & ATTR_NORMAL2) && (!(glState.vertexAttribPointersSet & ATTR_NORMAL2) || animated))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_NORMAL2 )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_NORMAL2, 3, GL_FLOAT, 0, glState.currentVBO->stride_normal, BUFFER_OFFSET(glState.currentVBO->ofs_normal + oldFrame * glState.currentVBO->size_normal));
		glState.vertexAttribPointersSet |= ATTR_NORMAL2;
	}

#ifdef USE_VERT_TANGENT_SPACE
	if ((attribBits & ATTR_TANGENT2) && (!(glState.vertexAttribPointersSet & ATTR_TANGENT2) || animated))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_TANGENT2 )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_TANGENT2, 3, GL_FLOAT, 0, glState.currentVBO->stride_tangent, BUFFER_OFFSET(glState.currentVBO->ofs_tangent + oldFrame * glState.currentVBO->size_normal)); // FIXME
		glState.vertexAttribPointersSet |= ATTR_TANGENT2;
	}

	if ((attribBits & ATTR_BITANGENT2) && (!(glState.vertexAttribPointersSet & ATTR_BITANGENT2) || animated))
	{
		GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_BITANGENT2 )\n");

		qglVertexAttribPointerARB(ATTR_INDEX_BITANGENT2, 3, GL_FLOAT, 0, glState.currentVBO->stride_bitangent, BUFFER_OFFSET(glState.currentVBO->ofs_bitangent + oldFrame * glState.currentVBO->size_normal)); // FIXME
		glState.vertexAttribPointersSet |= ATTR_BITANGENT2;
	}
#endif

}


shaderProgram_t *GLSL_GetGenericShaderProgram(int stage)
{
	shaderStage_t *pStage       = tess.xstages[stage];
	int           shaderAttribs = 0;

	if (tess.fogNum && pStage->adjustColorsForFog)
	{
		shaderAttribs |= GENERICDEF_USE_FOG;
	}

	if (pStage->bundle[1].image[0] && tess.shader->multitextureEnv)
	{
		shaderAttribs |= GENERICDEF_USE_LIGHTMAP;
	}

	switch (pStage->rgbGen)
	{
	case CGEN_LIGHTING_DIFFUSE:
		shaderAttribs |= GENERICDEF_USE_RGBAGEN;
		break;
	default:
		break;
	}

	switch (pStage->alphaGen)
	{
	case AGEN_LIGHTING_SPECULAR:
	case AGEN_PORTAL:
	case AGEN_FRESNEL:
		shaderAttribs |= GENERICDEF_USE_RGBAGEN;
		break;
	default:
		break;
	}

	if (pStage->bundle[0].tcGen != TCGEN_TEXTURE)
	{
		shaderAttribs |= GENERICDEF_USE_TCGEN_AND_TCMOD;
	}

	if (tess.shader->numDeforms && !ShaderRequiresCPUDeforms(tess.shader))
	{
		shaderAttribs |= GENERICDEF_USE_DEFORM_VERTEXES;
	}

	if (glState.vertexAttribsInterpolation > 0.0f && backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity)
	{
		shaderAttribs |= GENERICDEF_USE_VERTEX_ANIMATION;
	}

	if (pStage->bundle[0].numTexMods)
	{
		shaderAttribs |= GENERICDEF_USE_TCGEN_AND_TCMOD;
	}

	return &tr.genericShader[shaderAttribs];
}
#endif // RENDERER2C