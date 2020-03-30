/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
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
 * @file renderer2/tr_gl.c
 */

#include "tr_local.h"

/**
 * @brief GL_Bind
 * @param[in,out] image
 */
void GL_Bind(image_t *image)
{
	int texnum;

	if (!image)
	{
		Ren_Warning("GL_Bind: NULL image\n");
		image = tr.defaultImage;
	}
	else
	{
		Ren_LogComment("--- GL_Bind( %s ) ---\n", image->name);
	}

	texnum = image->texnum;

	/*if (r_noBind->integer && tr.blackImage)
	{
	    // performance evaluation option
	    texnum = tr.blackImage->texnum;
	    image  = tr.blackImage;
	}*/

	if (glState.currenttextures[glState.currenttmu] != texnum)
	{
		image->frameUsed                            = tr.frameCount;
		glState.currenttextures[glState.currenttmu] = texnum;
		glBindTexture(image->type, texnum);
	}
}


/**
 * @brief GL_Unbind
 */
void GL_Unbind()
{
	Ren_LogComment("--- GL_Unbind() ---\n");

	glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * @brief BindAnimatedImage
 * @param[in] bundle
 */
void BindAnimatedImage(textureBundle_t *bundle)
{
	int64_t index;

	if (bundle->isVideoMap)
	{
		ri.CIN_RunCinematic(bundle->videoMapHandle);
		ri.CIN_UploadCinematic(bundle->videoMapHandle);
		return;
	}

	if (bundle->numImages <= 1)
	{
		if (tess.surfaceShader->has_lightmapStage && (backEnd.refdef.rdflags & RDF_SNOOPERVIEW))
		{
			GL_Bind(tr.whiteImage);
		}
		else
		{
			GL_Bind(bundle->image[0]);
		}
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	//index   = Q_ftol(backEnd.refdef.floatTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);
	//index = (int64_t)(backEnd.refdef.floatTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);

	index   = (int64_t)(tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);
	index >>= FUNCTABLE_SIZE2; // ??! what is this? it does slow down the flames
	//index %= FUNCTABLE_SIZE;

	if (index < 0)
	{
		index = 0;              // may happen with shader time offsets
	}
	index %= bundle->numImages;

	if (tess.surfaceShader->has_lightmapStage && (backEnd.refdef.rdflags & RDF_SNOOPERVIEW))
	{
		GL_Bind(tr.whiteImage);
	}
	else
	{
		GL_Bind(bundle->image[index]);
	}
}

/*
 * @brief GL_TextureFilter
 * @param[in,out] image
 * @param[in] filterType
void GL_TextureFilter(image_t *image, filterType_t filterType)
{
    if (!image)
    {
        Ren_Warning("GL_TextureFilter: NULL image\n");
    }
    else
    {
        Ren_LogComment("--- GL_TextureFilter( %s ) ---\n", image->name);
    }

    if (image->filterType == filterType)
    {
        return;
    }

    // set filter type
    switch (image->filterType)
    {
    //case FT_DEFAULT:
    //glTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, gl_filter_min);
    //glTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, gl_filter_max);

    // set texture anisotropy
    //if(glConfig2.textureAnisotropyAvailable)
    //glTexParameterf(image->type, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_ext_texture_filter_anisotropic->value);
    //break;
    case FT_LINEAR:
        glTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    case FT_NEAREST:
        glTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;
    default:
        break;
    }
}
*/

/**
 * @brief GL_SelectTexture
 * @param[in] unit
 */
void GL_SelectTexture(int unit)
{
	if (glState.currenttmu == unit)
	{
		return;
	}

	if (unit >= 0 && unit <= 31)
	{
		glActiveTexture(GL_TEXTURE0 + unit);

		Ren_LogComment("glActiveTexture( GL_TEXTURE%i )\n", unit);
	}
	else
	{
		Ren_Drop("GL_SelectTexture: unit = %i", unit);
	}

	glState.currenttmu = unit;
}

/**
 * @brief GL_BlendFunc
 * @param[in] sfactor
 * @param[in] dfactor
 */
void GL_BlendFunc(GLenum sfactor, GLenum dfactor)
{
	if (glState.blendSrc != sfactor || glState.blendDst != dfactor)
	{
		glState.blendSrc = sfactor;
		glState.blendDst = dfactor;

		glBlendFunc(sfactor, dfactor);
	}
}

/**
 * @brief GL_ClearColor
 * @param[in] red
 * @param[in] green
 * @param[in] blue
 * @param[in] alpha
 */
void GL_ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	if (glState.clearColorRed != red || glState.clearColorGreen != green || glState.clearColorBlue != blue || glState.clearColorAlpha != alpha)
	{
		glState.clearColorRed   = red;
		glState.clearColorGreen = green;
		glState.clearColorBlue  = blue;
		glState.clearColorAlpha = alpha;

		glClearColor(red, green, blue, alpha);
	}
}

/**
 * @brief GL_ClearDepth
 * @param[in] depth
 */
void GL_ClearDepth(GLclampd depth)
{
	if (glState.clearDepth != depth)
	{
		glState.clearDepth = depth;

		glClearDepth(depth);
	}
}

/**
 * @brief GL_ClearStencil
 * @param[in] s
 */
void GL_ClearStencil(GLint s)
{
	if (glState.clearStencil != s)
	{
		glState.clearStencil = s;

		glClearStencil(s);
	}
}

/**
 * @brief GL_ColorMask
 * @param[in] red
 * @param[in] green
 * @param[in] blue
 * @param[in] alpha
 */
void GL_ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	if (glState.colorMaskRed != red || glState.colorMaskGreen != green || glState.colorMaskBlue != blue || glState.colorMaskAlpha != alpha)
	{
		glState.colorMaskRed   = red;
		glState.colorMaskGreen = green;
		glState.colorMaskBlue  = blue;
		glState.colorMaskAlpha = alpha;

		glColorMask(red, green, blue, alpha);
	}
}

/**
 * @brief GL_CullFace
 * @param[in] mode
 */
void GL_CullFace(GLenum mode)
{
	if (glState.cullFace != mode)
	{
		glState.cullFace = mode;

		glCullFace(mode);
	}
}

/**
 * @brief GL_DepthFunc
 * @param[in] func
 */
void GL_DepthFunc(GLenum func)
{
	if (glState.depthFunc != func)
	{
		glState.depthFunc = func;

		glDepthFunc(func);
	}
}

/**
 * @brief GL_DepthMask
 * @param[in] flag
 */
void GL_DepthMask(GLboolean flag)
{
	if (glState.depthMask != flag)
	{
		glState.depthMask = flag;

		glDepthMask(flag);
	}
}

/**
 * @brief GL_DrawBuffer
 * @param[in] mode
 */
void GL_DrawBuffer(GLenum mode)
{
	if (glState.drawBuffer != mode)
	{
		glState.drawBuffer = mode;

		glDrawBuffer(mode);
	}
}

/**
 * @brief GL_FrontFace
 * @param[in] mode
 */
void GL_FrontFace(GLenum mode)
{
	if (glState.frontFace != mode)
	{
		glState.frontFace = mode;

		glFrontFace(mode);
	}
}

/**
 * @brief GL_LoadModelViewMatrix
 * @param[in] m
 */
void GL_LoadModelViewMatrix(const mat4_t m)
{
	if (mat4_compare(GLSTACK_MVM, m))
	{
		return;
	}

	mat4_copy(m, GLSTACK_MVM);
	mat4_mult(GLSTACK_PM, GLSTACK_MVM, GLSTACK_MVPM);
}

/**
 * @brief GL_LoadProjectionMatrix
 * @param[in] m
 */
void GL_LoadProjectionMatrix(const mat4_t m)
{
	if (mat4_compare(GLSTACK_PM, m))
	{
		return;
	}

	mat4_copy(m, GLSTACK_PM);
	mat4_mult(GLSTACK_PM, GLSTACK_MVM, GLSTACK_MVPM);
}

/**
 * @brief GL_PushMatrix
 */
void GL_PushMatrix()
{
	glState.stackIndex++;

	if (glState.stackIndex >= MAX_GLSTACK)
	{
		glState.stackIndex = MAX_GLSTACK - 1;
		Ren_Drop("GL_PushMatrix: stack overflow = %i", glState.stackIndex);
	}
}

/**
 * @brief GL_PopMatrix
 */
void GL_PopMatrix()
{
	glState.stackIndex--;

	if (glState.stackIndex < 0)
	{
		glState.stackIndex = 0;
		Ren_Drop("GL_PopMatrix: stack underflow");
	}
}

/**
 * @brief GL_PolygonMode
 * @param[in] face
 * @param[in] mode
 */
void GL_PolygonMode(GLenum face, GLenum mode)
{
	if (glState.polygonFace != face || glState.polygonMode != mode)
	{
		glState.polygonFace = face;
		glState.polygonMode = mode;

		glPolygonMode(face, mode);
	}
}

/**
 * @brief GL_Scissor
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 */
void GL_Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	if (glState.scissorX != x || glState.scissorY != y || glState.scissorWidth != width || glState.scissorHeight != height)
	{
		glState.scissorX      = x;
		glState.scissorY      = y;
		glState.scissorWidth  = width;
		glState.scissorHeight = height;

		glScissor(x, y, width, height);
	}
}

/**
 * @brief GL_Viewport
 * @param[in] x
 * @param[in] y
 * @param[in] width
 * @param[in] height
 */
void GL_Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	if (glState.viewportX != x || glState.viewportY != y || glState.viewportWidth != width || glState.viewportHeight != height)
	{
		glState.viewportX      = x;
		glState.viewportY      = y;
		glState.viewportWidth  = width;
		glState.viewportHeight = height;

		glViewport(x, y, width, height);
	}
}

/**
 * @brief GL_PolygonOffset
 * @param[in] factor
 * @param[in] units
 */
void GL_PolygonOffset(float factor, float units)
{
	if (glState.polygonOffsetFactor != factor || glState.polygonOffsetUnits != units)
	{
		glState.polygonOffsetFactor = factor;
		glState.polygonOffsetUnits  = units;

		glPolygonOffset(factor, units);
	}
}

/**
 * @brief GL_Clear
 * @param[in] bits
 */
void GL_Clear(unsigned int bits)
{
	glClear(bits);
}

/**
 * @brief GL_Cull
 * @param[in] cullType
 */
void GL_Cull(int cullType)
{
	if (backEnd.viewParms.isMirror)
	{
		GL_FrontFace(GL_CW);
	}
	else
	{
		GL_FrontFace(GL_CCW);
	}

	// allow culling to be disabled
	if (r_noCull->integer)
	{
		glDisable(GL_CULL_FACE);
	}

	if (glState.faceCulling == cullType)
	{
		return;
	}

	glState.faceCulling = cullType;

	if (cullType == CT_TWO_SIDED)
	{
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_CULL_FACE);

		if (cullType == CT_BACK_SIDED)
		{
			GL_CullFace(GL_BACK);
		}
		else
		{
			GL_CullFace(GL_FRONT);
		}
	}
}

/**
 * @brief This routine is responsible for setting the most commonly changed state in Q3.
 * @param[in] stateBits
 */
void GL_State(uint32_t stateBits)
{
	uint32_t diff = stateBits ^ glState.glStateBits;

	if (!diff)
	{
		return;
	}

	// check depthFunc bits
	if (diff & GLS_DEPTHFUNC_BITS)
	{
		switch (stateBits & GLS_DEPTHFUNC_BITS)
		{
		default:
			GL_DepthFunc(GL_LEQUAL);
			break;
		case GLS_DEPTHFUNC_LESS:
			GL_DepthFunc(GL_LESS);
			break;
		case GLS_DEPTHFUNC_EQUAL:
			GL_DepthFunc(GL_EQUAL);
			break;
		}
	}

	// check blend bits
	if (diff & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))
	{
		GLenum srcFactor, dstFactor;

		if (stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))
		{
			switch (stateBits & GLS_SRCBLEND_BITS)
			{
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				srcFactor = GL_ONE;     // to get warning to shut up
				Ren_Drop("GL_State: invalid src blend state bits\n");
			}

			switch (stateBits & GLS_DSTBLEND_BITS)
			{
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				dstFactor = GL_ONE;     // to get warning to shut up
				Ren_Drop("GL_State: invalid dst blend state bits\n");
			}

			glEnable(GL_BLEND);
			GL_BlendFunc(srcFactor, dstFactor);
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}

	// check colormask
	if (diff & GLS_COLORMASK_BITS)
	{
		if (stateBits & GLS_COLORMASK_BITS)
		{
			GL_ColorMask((stateBits & GLS_REDMASK_FALSE) ? GL_FALSE : GL_TRUE,
			             (stateBits & GLS_GREENMASK_FALSE) ? GL_FALSE : GL_TRUE,
			             (stateBits & GLS_BLUEMASK_FALSE) ? GL_FALSE : GL_TRUE,
			             (stateBits & GLS_ALPHAMASK_FALSE) ? GL_FALSE : GL_TRUE);
		}
		else
		{
			GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}
	}

	// check depthmask
	if (diff & GLS_DEPTHMASK_TRUE)
	{
		if (stateBits & GLS_DEPTHMASK_TRUE)
		{
			GL_DepthMask(GL_TRUE);
		}
		else
		{
			GL_DepthMask(GL_FALSE);
		}
	}

	// fill/line mode
	if (diff & GLS_POLYMODE_LINE)
	{
		if (stateBits & GLS_POLYMODE_LINE)
		{
			GL_PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else
		{
			GL_PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	// depthtest
	if (diff & GLS_DEPTHTEST_DISABLE)
	{
		if (stateBits & GLS_DEPTHTEST_DISABLE)
		{
			glDisable(GL_DEPTH_TEST);
		}
		else
		{
			glEnable(GL_DEPTH_TEST);
		}
	}

	// alpha test - deprecated in OpenGL 3.0
#if 0
	if (diff & GLS_ATEST_BITS)
	{
		switch (stateBits & GLS_ATEST_BITS)
		{
		case GLS_ATEST_GT_0:
		case GLS_ATEST_LT_128:
		case GLS_ATEST_GE_128:
			//case GLS_ATEST_GT_CUSTOM:
			glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			break;

		default:
		case 0:
			glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			break;
		}
	}
#endif


	//if(diff & GLS_ATEST_BITS)
	//{
	//switch (stateBits & GLS_ATEST_BITS)
	//{
	//case 0:
	//glDisable(GL_ALPHA_TEST);
	//break;
	//case GLS_ATEST_GT_0:
	//glEnable(GL_ALPHA_TEST);
	//glAlphaFunc(GL_GREATER, 0.0f);
	//break;
	//case GLS_ATEST_LT_80:
	//glEnable(GL_ALPHA_TEST);
	//glAlphaFunc(GL_LESS, 0.5f);
	//break;
	//case GLS_ATEST_GE_80:
	//glEnable(GL_ALPHA_TEST);
	//glAlphaFunc(GL_GEQUAL, 0.5f);
	//break;
	//case GLS_ATEST_GT_CUSTOM:
	//// FIXME
	//glEnable(GL_ALPHA_TEST);
	//glAlphaFunc(GL_GREATER, 0.5f);
	//break;
	//default:
	//etl_assert(0);
	//break;
	//}
	//}


	// stenciltest
	if (diff & GLS_STENCILTEST_ENABLE)
	{
		if (stateBits & GLS_STENCILTEST_ENABLE)
		{
			glEnable(GL_STENCIL_TEST);
		}
		else
		{
			glDisable(GL_STENCIL_TEST);
		}
	}

	glState.glStateBits = stateBits;
}
