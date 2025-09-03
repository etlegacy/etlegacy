/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2024 ET:Legacy team <mail@etlegacy.com>
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
 * @file renderer/tr_shader_program.c
 * @brief Opengl 2.0 compatible shader program impl
 */

#include "tr_local.h"

#define MAX_SHADER_PROGRAMS 10

typedef struct
{
	shaderProgram_t programs[MAX_SHADER_PROGRAMS];
	shaderProgram_t *currentProgram;

	qboolean available;
} shaderProgramInfo_t;

static shaderProgramInfo_t shaderProgramInfo;

static shaderProgram_t *R_FindAvailableShaderProgram(void)
{
	for (int i = 0; i < MAX_SHADER_PROGRAMS; i++)
	{
		if (!shaderProgramInfo.programs[i].program)
		{
			return &shaderProgramInfo.programs[i];
		}
	}

	return NULL;
}

void R_UseShaderProgram(shaderProgram_t *program)
{
	if (program)
	{
		if (shaderProgramInfo.currentProgram != program)
		{
			glUseProgramObjectARB(program->program);
			shaderProgramInfo.currentProgram = program;
		}
	}
	else
	{
		glUseProgramObjectARB(0);
		shaderProgramInfo.currentProgram = NULL;
	}
}

GLint R_GetShaderProgramUniform(shaderProgram_t *program, const char *name)
{
	R_UseShaderProgram(program);
	return glGetUniformLocation(program->program, name);
}

shaderProgram_t *R_CreateShaderProgram(const char *vert, const char *frag)
{
	GLint           compiled;
	shaderProgram_t *program = R_FindAvailableShaderProgram();

	if (!program)
	{
		Ren_Fatal("Could not get a shader program from list\n");
	}

	program->vertexShader   = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	program->fragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

	glShaderSourceARB(program->vertexShader, 1, (const GLcharARB **)&vert, NULL);
	glShaderSourceARB(program->fragmentShader, 1, (const GLcharARB **)&frag, NULL);

	glCompileShaderARB(program->vertexShader);
	glCompileShaderARB(program->fragmentShader);

	glGetObjectParameterivARB(program->vertexShader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		GLint   blen = 0;
		GLsizei slen = 0;

		glGetShaderiv(program->vertexShader, GL_INFO_LOG_LENGTH, &blen);
		if (blen > 1)
		{
			GLchar *compiler_log;

			compiler_log = (GLchar *) Com_Allocate(blen);

			glGetInfoLogARB(program->vertexShader, blen, &slen, compiler_log);
			Ren_Fatal("Failed to compile the gamma vertex shader reason: %s\n", compiler_log);
		}
		else
		{
			Ren_Fatal("Failed to compile the gamma vertex shader\n");
		}
	}

	glGetObjectParameterivARB(program->fragmentShader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		Ren_Fatal("Failed to compile the gamma fragment shader\n");
	}

	program->program = glCreateProgramObjectARB();
	if (!program->program)
	{
		Ren_Fatal("Failed to create program\n");
	}

	glAttachObjectARB(program->program, program->vertexShader);
	glAttachObjectARB(program->program, program->fragmentShader);

	glLinkProgramARB(program->program);

	glGetObjectParameterivARB(program->program, GL_OBJECT_LINK_STATUS_ARB, &compiled);

	if (!compiled)
	{
		Ren_Fatal("Failed to link gamma shaders\n");
	}

	return program;
}

void R_DestroyShaderProgram(shaderProgram_t *program)
{
	if (!program || !program->program)
	{
		return;
	}

	if (program->vertexShader)
	{
		glDetachObjectARB(program->program, program->vertexShader);
		glDeleteObjectARB(program->vertexShader);
	}

	if (program->fragmentShader)
	{
		glDetachObjectARB(program->program, program->fragmentShader);
		glDeleteObjectARB(program->fragmentShader);
	}

	glDeleteObjectARB(program->program);
	Com_Memset(program, 0, sizeof(shaderProgram_t));
}

qboolean R_ShaderProgramsAvailable(void)
{
	return shaderProgramInfo.available;
}

void R_InitShaderPrograms(void)
{
	Com_Memset(shaderProgramInfo.programs, 0, sizeof(shaderProgram_t) * MAX_SHADER_PROGRAMS);

	if (!GLEW_ARB_fragment_program)
	{
		shaderProgramInfo.available = qfalse;
		Ren_Print("WARNING: R_InitShaderPrograms() skipped - no ARB_fragment_program\n");
		return;
	}

	shaderProgramInfo.available = qtrue;
}

void R_ShutdownShaderPrograms(void)
{
	R_UseShaderProgram(NULL);
	for (int i = 0; i < MAX_SHADER_PROGRAMS; i++)
	{
		R_DestroyShaderProgram(&shaderProgramInfo.programs[i]);
	}
}
